#include "frame.h"

#define inc(x) x = !x

extern int sock;

extern char next_to_send;
extern char frame_expected;
extern frame recv_f;
extern frame send_f;

void ConexaoRawSocket(char *device);
void socket_send(frame *f, int buf_len);
void socket_receive(frame *f, int buf_len);
void recebe_janela();
void envia_janela(char tam, char tipo, unsigned char *data, int type);
void clock_init();
void clock_stop();
