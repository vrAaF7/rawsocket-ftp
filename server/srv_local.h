#ifndef __KERMIT_SERVER_LOCAL__
#define __KERMIT_SERVER_LOCAL__

#include <limits.h>  // include a definição de PATH_MAX

#define ANSI_COLOR_GREEN   "\x1b[1;32m"
#define ANSI_COLOR_BLUE    "\x1b[1;34m"
#define ANSI_COLOR_RESET   "\x1b[0m"

extern char dir_atual[PATH_MAX];

void server_init_local();
char *server_lslocal(int a, int l);
int server_cdlocal(char *nome_dir);
long espaco_disponivel();

#endif
