#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "ConexaoRawSocket.h"

#include "server/srv_local.h"
#include "server/srv_remote.h"

int main(int argc, char **argv)
{
  if(argc == 1)
  {
    printf("Erro: especifique a interface como argumento.\n");
    exit(1);
  }

  // Inicializa a variável global que indica o diretório atual local
  server_init_local();

  // Inicializa o RawSocket na interface especificada pelo primeiro argumento
  ConexaoRawSocket(argv[1]);

  while(1)
  {
    recebe_janela();

    if(recv_f.control.tipo == FRAME_TYPE_CD)
      server_cd();
    else if(recv_f.control.tipo == FRAME_TYPE_LS)
      server_ls();
    else if(recv_f.control.tipo == FRAME_TYPE_GET)
      server_get();
    else if(recv_f.control.tipo == FRAME_TYPE_PUT)
      server_put();
  }
}
