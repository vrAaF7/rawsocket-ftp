#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "ConexaoRawSocket.h"

#include "client/cl_local.h"

int main(int argc, char **argv)
{
  if(argc == 1)
  {
    printf("Erro: especifique a interface como argumento.\n");
    exit(1);
  }

  // Inicializa a variável global que indica o diretório atual local
  init_local();

  // Inicializa o RawSocket na interface especificada pelo primeiro argumento
  ConexaoRawSocket(argv[1]);

  while(1)
  {
    printf("ftp> ");
    readInput();
  }
}
