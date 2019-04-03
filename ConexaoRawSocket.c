#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <net/ethernet.h>
#include <linux/if_packet.h>
#include <linux/if.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <netinet/in.h>  // definição não-implícita de htons
#include <unistd.h>  // write, read
#include <errno.h>
#include <fcntl.h>
#include <time.h>
#include <signal.h>
#include <sys/time.h>

#include "ConexaoRawSocket.h"

#define DANGEROUS_BYTE_1 0x81  // 1 no translate
#define DANGEROUS_BYTE_2 0x88  // 2 no translate
#define MAGIC_BYTE1 0xff  // byte temporário que substitui os bytes perigosos
#define MAGIC_BYTE2 0x00  // byte temporário que substitui os bytes perigosos

#define KERMIT_TIMEOUT_SECONDS 1

// Número do file descript do socket (global)
int sock;  

// Estrutura que define um tratador de sinal
struct sigaction action;

// Estrutura de inicialização to timer
struct itimerval timer;

// Relógio utilizado para medir o timeout
unsigned int relogio = 0;

// Tempo máximo da frame atual (até quando iremos esperar)
unsigned int frame_limit_time;

char next_to_send = 0;
char frame_expected = 0;

frame recv_f;
frame send_f;

void ConexaoRawSocket(char *device)
{
  int soquete;
  struct ifreq ir;
  struct sockaddr_ll endereco;
  struct packet_mreq mr;

  soquete = socket(AF_PACKET, SOCK_RAW, htons(ETH_P_ALL));  	/*cria socket*/
  if (soquete == -1) {
    printf("Erro no Socket. Sudo?\n");
    exit(-1);
  }

  memset(&ir, 0, sizeof(struct ifreq));  	/*dispositivo eth0*/

  /* Por que era sizeof(device) no código do Albini? Tava dando pra entrar com
     qualquer nome de interface aleatório e ainda assim capturar pacotes */
  memcpy(ir.ifr_name, device, strlen(device));

  if (ioctl(soquete, SIOCGIFINDEX, &ir) == -1) {
    printf("Erro no ioctl\n");
    exit(-1);
  }

  memset(&endereco, 0, sizeof(endereco)); 	/*IP do dispositivo*/
  endereco.sll_family = AF_PACKET;
  endereco.sll_protocol = htons(ETH_P_ALL);
  endereco.sll_ifindex = ir.ifr_ifindex;
  if (bind(soquete, (struct sockaddr *)&endereco, sizeof(endereco)) == -1) {
    printf("Erro no bind\n");
    exit(-1);
  }

  memset(&mr, 0, sizeof(mr));          /*Modo Promiscuo*/
  mr.mr_ifindex = ir.ifr_ifindex;
  mr.mr_type = PACKET_MR_PROMISC;
  if (setsockopt(soquete, SOL_PACKET, PACKET_ADD_MEMBERSHIP, &mr, sizeof(mr)) == -1)	{
    printf("Erro ao fazer setsockopt\n");
    exit(-1);
  }

  memset((void *) &send_f, 0, FRAME_SIZE);
  memset((void *) &recv_f, 0, FRAME_SIZE);

  // Define o socket global
  sock = soquete;
}

void socket_send(frame *f, int buf_len)
{
  for(int i = 0; i < f->control.tam; i++)
  {
    if(f->dados.bytes[i] == DANGEROUS_BYTE_1)
    {
      f->dados.bytes[i] = MAGIC_BYTE1;
      f->translate[i] = 1;
    }
    else if(f->dados.bytes[i] == DANGEROUS_BYTE_2)
    {
      f->dados.bytes[i] = MAGIC_BYTE2;
      f->translate[i] = 2;
    }
  }

  if(write(sock, f, buf_len) == -1)
  {
    if(errno != EINTR)
    {
      perror("Erro no send.\n");
      exit(1);
    }
  }
}

void socket_receive(frame *f, int buf_len)
{
  if(read(sock, f, buf_len) == -1)
  {
    if(errno != EINTR)
    {
      perror("Erro no recv.\n");
      exit(1);
    }
  }

  for(int i = 0; i < f->control.tam; i++)
  {
    if(f->translate[i] == 1)
    {
      f->dados.bytes[i] = DANGEROUS_BYTE_1;
      f->translate[i] = 0;
    }
    else if(f->translate[i] == 2)
    {
      f->dados.bytes[i] = DANGEROUS_BYTE_2;
      f->translate[i] = 0;
    }
  }
}

void recebe_janela()
{
  while(1)
  {
    // Inicia o timer
    clock_init();

    while(1)
    {
      // Chamada bloqueante (o kernel lida com isso, não é busy wait)
      socket_receive(&recv_f, FRAME_SIZE);
      
      if(recv_f.control.inicio == FRAME_INICIO)
      {
        clock_stop();
        break;
      }
    }

    // Verifica a paridade da frame recebida
    if(recv_f.paridade == calcula_paridade(&recv_f))
    {
      // Recebi a seq. esperada
      if(recv_f.control.seq == frame_expected)
      {
	// Recebi um NACK
        if(recv_f.control.tipo == FRAME_TYPE_NACK)
        {
          // Reenvia a última mensagem (estava no buffer)
          memset((void *) &recv_f, 0, FRAME_SIZE);
          socket_send(&send_f, FRAME_SIZE);
        }
        else  // Recebi um não-NACK
        {
          // Recebi uma frame não-NACK, incrementa a frame esperada
          inc(frame_expected);
          return;
        }
      }
    }
    else  // Erro de paridade
    {
      // Envia NACK
      printf("Erro de paridade. Enviado nack...\n");
      memset((void *) &recv_f, 0, FRAME_SIZE);
      build_frame(&send_f, 0, next_to_send, FRAME_TYPE_NACK, NULL, 0);
      socket_send(&send_f, FRAME_SIZE);
    }
  }
}

void envia_janela(char tam, char tipo, unsigned char *data, int type)
{
  build_frame(&send_f, tam, next_to_send, tipo, data, type);
  socket_send(&send_f, FRAME_SIZE);
  inc(next_to_send);
}

void tratador()
{
  relogio++;

  if(relogio >= frame_limit_time)
  {
    // Time out. Reenvia a última mensagem (estava no buffer)
    memset((void *) &recv_f, 0, FRAME_SIZE);
    socket_send(&send_f, FRAME_SIZE);
  }
}

// Inicializa o relógio
void clock_init()
{
  frame_limit_time = relogio + KERMIT_TIMEOUT_SECONDS;

  // Ativa o tratador
  action.sa_handler = tratador;

  //Função que permite a manipulação dos sinais POSIX
  sigemptyset (&action.sa_mask);
  action.sa_flags = 0;

  // Chamada de sistema, no caso de um sinal específico
  if (sigaction (SIGALRM, &action, 0) < 0) {
    perror ("Erro em sigaction: ") ;
    exit (1) ;
  }

  // Primeiro disparo em 1 segundo
  timer.it_value.tv_usec = 0;
  timer.it_value.tv_sec  = KERMIT_TIMEOUT_SECONDS;

  // Disparos a cada um segundo
  timer.it_interval.tv_usec = 0;
  timer.it_interval.tv_sec  = KERMIT_TIMEOUT_SECONDS;

  //Dispara o temporizador
  if (setitimer (ITIMER_REAL, &timer, 0) < 0)  { 
    perror ("Erro em setitimer: ") ;
    exit (1) ;
  }
}

// Encerra o timer e as interrupções
void clock_stop()
{
  timer.it_value.tv_usec = 0;
  timer.it_value.tv_sec  = 0;
  timer.it_interval.tv_usec = 0;
  timer.it_interval.tv_sec  = 0;
  setitimer (ITIMER_REAL, &timer, 0);
}
