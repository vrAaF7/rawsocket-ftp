#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>

#include "frame.h"
#include "ConexaoRawSocket.h"
#include "cl_local.h"

void cd(char *arg)
{
  // Envia a mensagem inicial para requisição do cd remoto
  envia_janela(strlen(arg), FRAME_TYPE_CD, (unsigned char *) arg, DATA_CHAR);

  while(1)
  {
    recebe_janela();

    int tipo = recv_f.control.tipo;

    // Permissão negada ou diretório não existe (no servidor)
    if(tipo == FRAME_TYPE_ERROR)
    {
      if(recv_f.dados.bytes[0] == FRAME_ERROR_NO_SUCH_FILE_OR_DIRECTORY)
        printf("Remoto: diretório não existe.\n");
      else if(recv_f.dados.bytes[0] == FRAME_ERROR_PERMISSION)
        printf("Remoto: permissão negada.\n");

      return;
    }
    else if(tipo == FRAME_TYPE_OK)
    {
      printf("Remoto: Sucesso.\n");
      return;
    }
  }
}

void ls(int a, int l)
{
  // Envia a mensagem inicial para requisição do ls remoto
  unsigned char msg[2];
  msg[0] = (unsigned char) a;
  msg[1] = (unsigned char) l;
  envia_janela(sizeof(msg), FRAME_TYPE_LS, msg, DATA_CHAR);

  while(1)
  {
    recebe_janela();

    if(recv_f.control.tipo == FRAME_TYPE_FIM)
    {
      return;
    }
    else if(recv_f.control.tipo == FRAME_TYPE_SHOW)
    {
      printf("%c", recv_f.dados.bytes[0]);

      // Envia ACK do caractere
      envia_janela(0, FRAME_TYPE_ACK, NULL, DATA_CHAR);
    }
  }
}

void get(char *arg)
{
  FILE *w;

  // Envia a mensagem inicial para requisição do get. A mensagem inicial contém
  // o nome do arquivo. Note que a string pode não terminar em 0 (todos os 31
  // bytes usados), mas isso é irrelevante porque existe padding iniciado com 0
  // depois dos dados. Testado com tamanho exatamente 31 e funciona
  envia_janela(strlen(arg), FRAME_TYPE_GET, (unsigned char *) arg, DATA_CHAR);

  // Espera pela mensagem de tamanho ou erro
  while(1)
  {
    recebe_janela();
 
    if(recv_f.control.tipo == FRAME_TYPE_ERROR)
    { 
      if(recv_f.dados.bytes[0] == FRAME_ERROR_NO_SUCH_FILE_OR_DIRECTORY)
        printf("Remoto: arquivo não existe.\n");
      else if(recv_f.dados.bytes[0] == FRAME_ERROR_PERMISSION)  // implementar?
        printf("Remoto: permissão negada.\n");

      return;
    }
    else if(recv_f.control.tipo == FRAME_TYPE_TAM)
    {
      // Recebeu o tamanho do arquivo, enviado pelo servidor. Podemos abrir
      // ou criar o arquivo local para começar a escrita

      long espaco = espaco_disponivel();

      if(recv_f.dados.long_int <= espaco)
      {
        char full_write_name[PATH_MAX];
        strcpy(full_write_name, (char *) &dir_atual);
        strcat(full_write_name, "/");
        strcat(full_write_name, arg);

        // Opção 'w+' cria o arquivo caso ele não exista
        w = fopen(full_write_name, "w+");

        if(w == NULL)
        {
          printf("Erro: fopen\n");
          exit(1);
        }

        // Envia OK para o servidor. O servidor começará então a enviar o arq.
        envia_janela(0, FRAME_TYPE_OK, NULL, DATA_CHAR);

        break;  // Sai do while(1) e espera os dados
      }
      else
      {
        // Sinaliza para o servidor que não há espaço e lê o próx. comando
        unsigned char msg = FRAME_ERROR_NOT_ENOUGH_SPACE;

        // Mensagem do tipo erro com conteúdo (dados): "não há espaço"
        envia_janela(sizeof(msg), FRAME_TYPE_ERROR, &msg, DATA_CHAR);
        return;
      }
    }
  }

  while(1)
  {
    recebe_janela();

    if(recv_f.control.tipo == FRAME_TYPE_DATA)
    {
      // Escreve os bytes recebidos no arquivo 'w'
      fwrite(recv_f.dados.bytes, 1, recv_f.control.tam, w);

      // Envia ACK para o servidor, confirmando o recebimento dos dados
      envia_janela(0, FRAME_TYPE_ACK, NULL, DATA_CHAR);
    }
    else if(recv_f.control.tipo == FRAME_TYPE_FIM)
    {
      fclose(w);
      return;
    }
  }
}

void put(char *arg)
{
  FILE *r;

  char full_read_name[PATH_MAX];
  strcpy(full_read_name, (char *) &dir_atual);
  strcat(full_read_name, "/");
  strcat(full_read_name, arg);

  r = fopen(full_read_name, "r");
  
  unsigned char read_buffer[31];

  // Erro ao abrir o arquivo
  if(r == NULL)
  {
    // File not found
    if(errno == ENOENT)
    {
      printf("Erro: arquivo não existe.\n");
      return;
    }
    else
    {
      printf("Erro: outro erro em fopen.\n");
      exit(1);
    }
  }

  // Obtém o tamanho do arquivo
  fseek(r, 0, SEEK_END);
  long file_size = ftell(r);
  fseek(r, 0, SEEK_SET);

  // Envia a mensagem inicial para a requisição do put, contendo o nome do arq.
  envia_janela(strlen(arg), FRAME_TYPE_PUT, (unsigned char *) arg, DATA_CHAR);
  
  // Espera pelo OK da mensagem acima
  while(1)
  {
    recebe_janela();

    if(recv_f.control.tipo == FRAME_TYPE_OK)
    {
      // Envia mensagem de tamanho
      envia_janela(sizeof(long), FRAME_TYPE_TAM, (unsigned char*) &file_size,
                  DATA_LONG);

      break;
    }
  }

  while(1)
  {
    recebe_janela();
    
    int tipo = recv_f.control.tipo;

    // Recebeu frame do tipo FRAME_TYPE_ERROR. Como o único erro que o servidor
    // pode mandar é 'não há espaço', não precisamos verificar os dados
    if(tipo == FRAME_TYPE_ERROR)
    {
      printf("Servidor: não há espaço disponível.\n");
      return;
    }
    else if(tipo == FRAME_TYPE_OK || tipo == FRAME_TYPE_ACK)
    {
      int bytes_read = fread(read_buffer, 1, 31, r);

      // Chegamos no fim do arquivo. Manda uma mensagem de fim
      if(bytes_read == 0)
      {
        envia_janela(0, FRAME_TYPE_FIM, NULL, 0);
        fclose(r);
        return;
      }
      else  // Não chegamos no fim do arquivo
      {
        envia_janela(bytes_read, FRAME_TYPE_DATA, read_buffer, DATA_CHAR);
      }
    }
  }
}
