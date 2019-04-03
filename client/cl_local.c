#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

// Bibliotecas para manipulação de diretórios e arquivos
#include <dirent.h> 
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <pwd.h>
#include <grp.h>
#include <locale.h>
#include <errno.h>
#include <sys/statvfs.h>

#include "client/cl_local.h"
#include "client/cl_remote.h"

struct statvfs stat_espaco;  // Para calcular o espaço disponível em disco

// Armazena o diretório atual (corrente). PATH_MAX é definido em limits.h (4096)
char dir_atual[PATH_MAX];

// Inicializa com o diretório atual
void init_local() {
  getcwd(dir_atual, sizeof(dir_atual));
  setlocale(LC_ALL, "");
}

// ls local
void lslocal(int a, int l) {
  // Struct que armazena os arquivos do diretório
  struct dirent **lista_arquivos;

  // Struct que armazena as informações dos arquivos do diretório (ls -l)
  struct stat fileStat;

  // Struct que armazena o nome do usuário pelo user id (2ª coluna ls -l)
  struct passwd *pwd;

  // Struct que armazena o nome do grupo pelo group id (3ª coluna ls -l)
  struct group *group;

  // Struct que armazena a data da última modificação do arquivo
  struct tm *ts;
  
  // String que armazena a data formatada
  char data[80];

  // Inteiro que indica a quantidade de arquivos no diretório atual
  int num_arquivos, i;

  // Variável utilizada para calcular o tamanho total de blocos
  // Onde: bloco é a alocação total no disco para determinado arquivo
  // O tamanho do bloco é obtido em 1024 bytes, mas o ls -l utiliza 512 bytes
  // por isso, após ser calculado, o tamanho é dividido por 2
  int soma;

  // Scandir: função que insere os arquivos do diretório indicado (dir_atual)
  // em uma lista, retornando o seu tamanho (número de arquivos)
  // alphasort: formata a lista para ordem alfabéticaa
  num_arquivos = scandir(dir_atual, &lista_arquivos, NULL, alphasort);
  
  // Caso o diretório indicado não exista, ou o computador não tenha memória
  // suficiente para completar a operação, ou se o caminho indicado não é um
  // diretório, retorna em erro (ENOENT, ENOMEM, ENOTDIR)
  if(num_arquivos < 0) {
    printf("Erro: scandir\n");
    exit(1);
  }

  char full_path[PATH_MAX];

  // Não existe -l na chamada
  if(!l) {
    for(i = 0; i < num_arquivos; i++) {

      // Se a opção -a estiver ligada, imprime este arquivo, já que com -a
      // imprimimos todos os arquivos. Caso não esteja e o arquivo não comece
      // com '.', também imprime. Feito desse modo pra usar menos ifs
      if(a || lista_arquivos[i]->d_name[0] != '.') {
        sprintf(full_path, "%s/%s", dir_atual, lista_arquivos[i]->d_name);
        if(stat(full_path, &fileStat) < 0) {
          printf("Erro: fileStat\n");
          exit(1);
        }

        // Imprime nome do arquivo
        if(S_ISDIR(fileStat.st_mode)) {  // É arquivo (imprime azul)
          printf(ANSI_COLOR_BLUE);
          printf("%s\n", lista_arquivos[i]->d_name);
          printf(ANSI_COLOR_RESET);
        }
        else if(fileStat.st_mode & S_IXUSR)  // É executável (imprime verde)
        {
          printf(ANSI_COLOR_GREEN);
          printf("%s\n", lista_arquivos[i]->d_name);
          printf(ANSI_COLOR_RESET);
        }
        else  // É de qualquer outro tipo
          printf("%s\n", lista_arquivos[i]->d_name);
      }
    free(lista_arquivos[i]);
    }
  }
  else {
    // Caso seja ls -l, utiliza uma struct stat, que contém os status de cada
    // arquivo dentro do diretório

    // Percorre a lista, e soma o tamanho de bloco de cada arquivo
    soma = 0;
    for(i = 0; i < num_arquivos; i++) {
      if(a || lista_arquivos[i]->d_name[0] != '.') {
        sprintf(full_path, "%s/%s", dir_atual, lista_arquivos[i]->d_name);
        if(stat(full_path, &fileStat) < 0) {
          printf("Erro: fileStat\n");
          exit(1);
        }
        soma += (int)fileStat.st_blocks;
      }
    }

    // Como o tamanho avaliado no stat é em 1024 bytes, e o ls -l imprime em
    // 512 bytes, é necessário dividir por 2
    printf("total %d\n", soma/2);

    // Percorre a lista e imprime as informações sobre permissões, número de
    // links, nome de usuário, nome de grupo, tamanho do arquivo e data da
    // última modificação
    for(int i = 0; i < num_arquivos; i++) {

      if(a || lista_arquivos[i]->d_name[0] != '.') {
        sprintf(full_path, "%s/%s", dir_atual, lista_arquivos[i]->d_name);
        if(stat(full_path, &fileStat) < 0) {
          printf("Erro: fileStat\n");
          exit(1);
        }
        // Imprime as permissões
        printf( (S_ISDIR(fileStat.st_mode))  ? "d" : "-");
        printf( (fileStat.st_mode & S_IRUSR) ? "r" : "-");
        printf( (fileStat.st_mode & S_IWUSR) ? "w" : "-");
        printf( (fileStat.st_mode & S_IXUSR) ? "x" : "-");
        printf( (fileStat.st_mode & S_IRGRP) ? "r" : "-");
        printf( (fileStat.st_mode & S_IWGRP) ? "w" : "-");
        printf( (fileStat.st_mode & S_IXGRP) ? "x" : "-");
        printf( (fileStat.st_mode & S_IROTH) ? "r" : "-");
        printf( (fileStat.st_mode & S_IWOTH) ? "w" : "-");
        printf( (fileStat.st_mode & S_IXOTH) ? "x " : "- ");  

        // Imprime o número de links   
        printf("%2d ", (int)fileStat.st_nlink);

        // Obtém o nome do usuário pelo user id (uid)
        pwd = getpwuid(fileStat.st_uid);
        printf("%s ", pwd->pw_name);

        // Obtém o nome do grupo pelo group id (gid)
        group = getgrgid(fileStat.st_gid);
        printf("%s ", group->gr_name);

        // Imprime o tamanho do arquivo
        printf("%5.ld ", (long int)fileStat.st_size);
        
        // Imprime a data formatada ( <mês abreviado> <dia> <hh:mm> )
        ts = localtime(&(fileStat.st_mtime));
        strftime(data, sizeof(data), "%b %d %H:%M", ts);
        printf("%s ", data);
        
        // Imprime nome do arquivo
        if(S_ISDIR(fileStat.st_mode)) {  // É arquivo (imprime azul)
          printf(ANSI_COLOR_BLUE);
          printf("%s\n", lista_arquivos[i]->d_name);
          printf(ANSI_COLOR_RESET);
        }
        else if(fileStat.st_mode & S_IXUSR)  // É executável (imprime verde)
        {
          printf(ANSI_COLOR_GREEN);
          printf("%s\n", lista_arquivos[i]->d_name);
          printf(ANSI_COLOR_RESET);
        }
        else  // É de qualquer outro tipo
          printf("%s\n", lista_arquivos[i]->d_name);
      }
      
      // Libera memória do elemento da lista
      free(lista_arquivos[i]);
    }
  }

  // Libera memória da lista
  free(lista_arquivos);
}

// cd local
void cdlocal(char *nome_dir) {
  // Ponteiro para o diretório que será aberto
  DIR *dir;
  
  // String utilizada para verificar se o diretório existe
  char nome_dir_atual[80];
  char nome_dir_atual_resolvido[80];

  // Concatena o nome diretório indicado ao salvo na variável global dir_atual
  // e armazena na variável auxiliar nome_dir_atual
  strcpy(nome_dir_atual, dir_atual);
  strcat(nome_dir_atual, "/");
  strcat(nome_dir_atual, nome_dir);

  // Converte path relativo pra path real
  realpath(nome_dir_atual, nome_dir_atual_resolvido);
  
  // Tenta abrir o diretório indicado
  dir = opendir(nome_dir_atual_resolvido);

  // Caso opendir retorne em erro, avisa o usuário e volta à linha de comando
  if(dir == NULL) {
    if(errno == EACCES) {  // EACCES não testado
      printf("Erro: permissão negada.\n");
      return;
    }
    else if(errno == ENOENT) {
      printf("Erro: diretório não existe.\n");
      return;
    }
  }
  // Caso retorne com sucesso, altera a variável global dir_atual com o novo
  // nome do path
  else
  {
    closedir(dir);
    strcpy(dir_atual, nome_dir_atual_resolvido);
  }
}

void readInput() {
  char *linha = malloc(sizeof(char) * 256);
  char *arg1 = malloc(sizeof(char) * 32);
  char *arg2 = malloc(sizeof(char) * 32);
  char *arg3 = malloc(sizeof(char) * 32);

  fgets(linha, 256, stdin);

  sscanf(linha, "%s %s %s", arg1, arg2, arg3);

  if(strcmp(arg1, "!ls") == 0) {

    int a = 0;
    int l = 0;
    
    if(strcmp(arg2, "-a") == 0 || strcmp(arg3, "-a") == 0)
      a = 1;
    
    if(strcmp(arg2, "-l") == 0 || strcmp(arg3, "-l") == 0)
      l = 1;

    if(strcmp(arg2, "-al") == 0 || strcmp(arg2, "-la") == 0) {
      a = 1;
      l = 1;
    }

    lslocal(a, l);
  }
  else if(strcmp(arg1, "!cd") == 0) {  
    cdlocal(arg2);
  }
  else if(strcmp(arg1, "cd") == 0) {
    if(strlen(arg2) <= 31)
      cd(arg2);
    else
      printf("O tamanho máximo do nome do diretório é 31 caracteres.\n");
  }
  else if(strcmp(arg1, "ls") == 0) {

    // arrumar isso
    int a = 0;
    int l = 0;
    
    if(strcmp(arg2, "-a") == 0 || strcmp(arg3, "-a") == 0)
      a = 1;
    
    if(strcmp(arg2, "-l") == 0 || strcmp(arg3, "-l") == 0)
      l = 1;

    if(strcmp(arg2, "-al") == 0 || strcmp(arg2, "-la") == 0) {
      a = 1;
      l = 1;
    }

    ls(a, l);
  }
  else if(strcmp(arg1, "get") == 0) {
    if(strlen(arg2) <= 31)
      get(arg2);
    else
      printf("O tamanho máximo do nome de arquivo é 31 caracteres.\n");
  }
  else if(strcmp(arg1, "put") == 0 || strcmp(arg1, "send") == 0) {
    if(strlen(arg2) <= 31)
      put(arg2);
    else
      printf("O tamanho máximo do nome de arquivo é 31 caracteres.\n");
  }
  else if(strcmp(arg1, "!pwd") == 0) {  // Print working directory
    printf("%s\n", dir_atual);
  }
  else if(strcmp(arg1, "exit") == 0) { // Encerra o cliente
    printf("Goodbye.\n");
    exit(0);
  }
  else
    printf("Comando não reconhecido.\n");

  free(linha);
  free(arg1);
  free(arg2);
  free(arg3);
}

long espaco_disponivel() {

  if(statvfs((const char *) &dir_atual, &stat_espaco) == -1)  { // erro
    printf("Erro: statvfs\n");
    exit(1);
  }

  return stat_espaco.f_bsize * stat_espaco.f_bfree;
}
