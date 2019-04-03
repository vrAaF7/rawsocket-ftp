#ifndef __KERMIT_CLIENT_LOCAL__
#define __KERMIT_CLIENT_LOCAL__

#include <limits.h>  // inclui a definição de PATH_MAX

#define ANSI_COLOR_GREEN   "\x1b[1;32m"
#define ANSI_COLOR_BLUE    "\x1b[1;34m"
#define ANSI_COLOR_RESET   "\x1b[0m"

extern char dir_atual[PATH_MAX];

void init_local();
void lslocal(int a, int l);
void cdlocal(char *nome_dir);
void readInput();
long espaco_disponivel();

#endif
