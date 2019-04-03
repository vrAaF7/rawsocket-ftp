#ifndef __KERMIT_FRAME__
#define __KERMIT_FRAME__

// Marcador de início (0111.1110)
#define FRAME_INICIO 0x7e 

// Máscara de 1 byte nulo (0000.0000)
#define MASK_NULL 0x0 

// Máscara para acessar somente os 3 bits mais significativos de um elemento 
// de 6 bits
#define MASK_MAIS_SIG 0x38 

// Máscara para acessar somente os 3 bits menos significativos de um elemento
// de 6 bits
#define MASK_MENOS_SIG 0x7

// Tamanho da frame, em bytes (considerando o alinhamento em memória)
#define FRAME_SIZE 72

// Definições dos tipos das mensagens
#define FRAME_TYPE_ACK   0x0
#define FRAME_TYPE_TAM   0x2
#define FRAME_TYPE_OK    0x3
#define FRAME_TYPE_CD    0x6
#define FRAME_TYPE_LS    0x7
#define FRAME_TYPE_GET   0x8
#define FRAME_TYPE_PUT   0x9
#define FRAME_TYPE_FIM   0xa
#define FRAME_TYPE_SHOW  0xc
#define FRAME_TYPE_DATA  0xd
#define FRAME_TYPE_ERROR 0xe
#define FRAME_TYPE_NACK  0xf

// Definições dos códigos de erros (int? verificar)
#define FRAME_ERROR_NO_SUCH_FILE_OR_DIRECTORY 1
#define FRAME_ERROR_PERMISSION                2
#define FRAME_ERROR_NOT_ENOUGH_SPACE          3

// Tipos de dados (para acessar o union da frame)
#define DATA_CHAR 0
#define DATA_LONG 1

// Struct de controle: contém todos os bits que não são dados. Tamanho: 3 bytes
typedef struct controle_s {
  unsigned inicio:8;
  unsigned tam:5;
  unsigned seq:6;
  unsigned tipo:5;
} controle;

// Struct que contém a mensagem toda. Tamanho: 66 bytes
typedef struct frame_s {
  controle control;          // 3 bytes (alinhado pra 4?)
  union { // acomoda o tamanho máximo da union (31 bytes) (alinhado pra 32?)
    unsigned char bytes[31];
    unsigned long long_int;
  } dados;
  unsigned char paridade;  // 1 byte (alinhado pra 4?)
  unsigned char translate[31];  // 31 bytes (alinhado pra 32?)
} frame;

unsigned char calcula_paridade(frame *f);

void build_frame(frame *f, char tam, char seq, char tipo, unsigned char *data,
                 int data_type);

void print_frame(frame *f);

void print_frame_bits(frame *f);

#endif
