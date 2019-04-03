#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>

#include "ConexaoRawSocket.h"

#include "server/srv_local.h"

void server_cd()
{
  unsigned char cd_return = (unsigned char) server_cdlocal((char *)
                                                           recv_f.dados.bytes);

  if(cd_return != 0)  // Erro ao mudar de diretório
  {
    printf("Servidor: erro ao mudar de diretório.\n");

    // Envia mensagem de erro
    envia_janela(sizeof(char), FRAME_TYPE_ERROR, &cd_return, DATA_CHAR);
  }
  else  // cd_return == 0
  {  
    printf("Servidor: meu diretório agora é '%s'.\n", dir_atual);

    // Envia a mensagem OK
    envia_janela(sizeof(char), FRAME_TYPE_OK, &cd_return, DATA_CHAR);
  }
}

void server_ls()
{
  unsigned char *buf = (unsigned char *) server_lslocal(recv_f.dados.bytes[0],
                                                        recv_f.dados.bytes[1]);

  int buf_pos = 0;
  int buf_max = strlen((char *) buf);

  // Envia primeira mensagem de dados do ls
  envia_janela(sizeof(char), FRAME_TYPE_SHOW, &buf[buf_pos], DATA_CHAR);
  buf_pos++;

  while(buf_pos < buf_max)
  {
    recebe_janela();

    int tipo = recv_f.control.tipo;

    if(tipo == FRAME_TYPE_LS || tipo == FRAME_TYPE_ACK)
    {
      // Envia caractere e prossegue para o próximo
      envia_janela(sizeof(char), FRAME_TYPE_SHOW, &buf[buf_pos], DATA_CHAR);
      buf_pos++;
    }
  }

  // Envia a mensagem de fim
  envia_janela(0, FRAME_TYPE_FIM, NULL, DATA_CHAR);
  free(buf);
}

void server_get()
{
  FILE *r;

  char full_read_name[PATH_MAX];
  strcpy(full_read_name, (char *) &dir_atual);
  strcat(full_read_name, "/");
  strcat(full_read_name, (char *) recv_f.dados.bytes);

  r = fopen(full_read_name, "r");
  
  unsigned char read_buffer[31];
  memset(read_buffer, '\0', 31);

  // Erro ao abrir o arquivo. Envia mensagem de erro e termina
  if(r == NULL)
  {
    // File not found
    if(errno == ENOENT)
    {
      printf("Erro: arquivo não existe.\n");
      unsigned char msg = FRAME_ERROR_NO_SUCH_FILE_OR_DIRECTORY;
      envia_janela(sizeof(char), FRAME_TYPE_ERROR, &msg, DATA_CHAR);
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

  envia_janela(sizeof(long), FRAME_TYPE_TAM, (unsigned char*) &file_size,
               DATA_LONG);

  while(1)
  {
    recebe_janela();
    
    int tipo = recv_f.control.tipo;

    if(tipo == FRAME_TYPE_ERROR)
    {
      printf("Cliente: não há espaço disponível.\n");
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

      memset(read_buffer, '\0', 31);
    }
  }
}

void server_put()
{
  FILE *w = NULL;

  char nome_local[31];
  strcpy(nome_local, (char *) recv_f.dados.bytes);

  // Envia OK para o cliente, confirmando o recebimento da mensagem
  // 'FRAME_TYPE_PUT "nome do arquivo". Note que nesta função temos 2 OKs
  envia_janela(0, FRAME_TYPE_OK, NULL, DATA_CHAR);

  // Espera pela mensagem de tamanho
  while(1)
  {
    recebe_janela();

    if(recv_f.control.tipo == FRAME_TYPE_TAM)
    {
      // Recebeu o tamanho do arquivo, enviado pelo cliente. Podemos abrir
      // ou criar o arquivo local para começar a escrita
      long espaco = espaco_disponivel();

      if(recv_f.dados.long_int <= espaco)
      {
        char full_write_name[PATH_MAX];
        strcpy(full_write_name, (char *) &dir_atual);
        strcat(full_write_name, "/");
        strcat(full_write_name, nome_local);

        // Opção 'w+' cria o arquivo caso ele não exista
        w = fopen(full_write_name, "w+");

        if(w == NULL)
        {
          printf("Erro: fopen\n");
          exit(1);
        }

        // Envia OK para o cliente. O cliente começará então a enviar o arq.
        envia_janela(0, FRAME_TYPE_OK, NULL, DATA_CHAR);

        break;
      }
      else
      {
        // Sinaliza para o cliente que não há espaço a finaliza a função
        unsigned char msg = FRAME_ERROR_NOT_ENOUGH_SPACE;

        // Mensagem do tipo erro com conteúdo (dados): "não há espaço"
        envia_janela(sizeof(msg), FRAME_TYPE_ERROR, &msg, DATA_CHAR);
        return;
      }
    }
  }

  // Espera pelas mensagens de dados ou fim
  while(1)
  {
    recebe_janela();

    if(recv_f.control.tipo == FRAME_TYPE_DATA)
    {
      // Escreve os bytes recebidos no arquivo 'w'
      fwrite(recv_f.dados.bytes, recv_f.control.tam, 1, w);

      // Envia ACK para o cliente, confirmando o recebimento dos dados
      envia_janela(0, FRAME_TYPE_ACK, NULL, DATA_CHAR);
    }
    else if(recv_f.control.tipo == FRAME_TYPE_FIM)
    {
      fclose(w);
      return;
    }
  }
}
