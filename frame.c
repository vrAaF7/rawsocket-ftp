#include <stdlib.h>
#include <string.h>

#include <stdio.h>

#include "frame.h"

unsigned char calcula_paridade(frame *f) {
  int i;
  unsigned char bits, merge_tam_seq, merge_seq_tipo; // 8 bits
  unsigned char paridade;

  // Há uma junção dos 5 bits do tamanho com os 3 mais significativos da 
  // sequência, por exemplo:
  // Se tamanho = 00011 e sequencia = 110001
  // Juntando em 8 bits = 0001.1110

  // 110001 and 111000 == 0011.0000 >> 3 == 0000.0110 
  bits = (MASK_MAIS_SIG & f->control.seq) >> 3;
  
  // 0000.0000 or 00011 == 0000.0011
  merge_tam_seq = MASK_NULL | f->control.tam;
 
  // (0000.0011 << 3) == 0001.1000 or 0000.0110 == 0001.1110
  merge_tam_seq = (merge_tam_seq << 3) | bits;
  
  // Há uma junção dos 3 bits menos significativos da sequência com os 5 bits
  // de tipo, por exemplo:
  // Se sequência = 110001 e tipo = 01100
  // Juntando em 8 bits = 0010.1100

  // 000111 and 110001 == 0000.0001 << 5 == 0010.0000
  bits = (MASK_MENOS_SIG & f->control.seq) << 5;

  // 0010.0000 or 01100 == 0010.1100
  merge_seq_tipo = bits | f->control.tipo;

  // Calcula a paridade utilizando XOR:
  // Inicialmente, faz XOR de 8 bits de merge_tam_seq e merge_seq_tipo
  // Logo após, calcula o XOR de cada byte dos dados, que equivale a cada
  // posição do vetor unsigned char de dados
  paridade = merge_tam_seq ^ merge_seq_tipo;
  for(i = 0; i < f->control.tam; i++)
    paridade ^= f->dados.bytes[i];
  return(paridade);
}

void build_frame(frame *f, char tam, char seq, char tipo, unsigned char *data,
                 int type)
{
  f->control.inicio = FRAME_INICIO;
  f->control.tam = tam;
  f->control.seq = seq;
  f->control.tipo = tipo;
  memset(&f->dados.bytes, '\0', sizeof(f->dados.bytes));
  if(data != NULL)
  {
    if(type == DATA_CHAR)
      memcpy(&f->dados.bytes, data, tam);
    else  // DATA_LONG
      memcpy(&f->dados.long_int, data, tam);
  }

  f->paridade = calcula_paridade(f);
  memset(&f->translate, '\0', sizeof(f->translate));
}

// Debug
void print_frame_bits(frame *f) {
  printf("Tam.: %d\n", f->control.tam);
  printf("Seq.: %d\n", f->control.seq);
  printf("Tipo: %#X\n", f->control.tipo);

  printf("|");
  printf("%c%c%c%c%c%c%c%c",
         (f->control.inicio & 0x80 ? '1' : '0'),
         (f->control.inicio & 0x40 ? '1' : '0'),
         (f->control.inicio & 0x20 ? '1' : '0'),
         (f->control.inicio & 0x10 ? '1' : '0'),
         (f->control.inicio & 0x08 ? '1' : '0'),
         (f->control.inicio & 0x04 ? '1' : '0'),
         (f->control.inicio & 0x02 ? '1' : '0'),
         (f->control.inicio & 0x01 ? '1' : '0'));
  printf("|");
  printf("%c%c%c%c%c",
         (f->control.tam & 0x10 ? '1' : '0'),
         (f->control.tam & 0x08 ? '1' : '0'),
         (f->control.tam & 0x04 ? '1' : '0'),
         (f->control.tam & 0x02 ? '1' : '0'),
         (f->control.tam & 0x01 ? '1' : '0'));
  printf("|");
  printf("%c%c%c%c%c%c",
         (f->control.seq & 0x20 ? '1' : '0'),
         (f->control.seq & 0x10 ? '1' : '0'),
         (f->control.seq & 0x08 ? '1' : '0'),
         (f->control.seq & 0x04 ? '1' : '0'),
         (f->control.seq & 0x02 ? '1' : '0'),
         (f->control.seq & 0x01 ? '1' : '0'));
  printf("|");
  printf("%c%c%c%c%c",
         (f->control.tipo & 0x10 ? '1' : '0'),
         (f->control.tipo & 0x08 ? '1' : '0'),
         (f->control.tipo & 0x04 ? '1' : '0'),
         (f->control.tipo & 0x02 ? '1' : '0'),
         (f->control.tipo & 0x01 ? '1' : '0'));
  printf("|\nDADOS:\n");
  for(int i = 0; i < 31; i++) {
    printf("%d: %c%c%c%c%c%c%c%c\n", i,
           (f->dados.bytes[i] & 0x80 ? '1' : '0'),
           (f->dados.bytes[i] & 0x40 ? '1' : '0'),
           (f->dados.bytes[i] & 0x20 ? '1' : '0'),
           (f->dados.bytes[i] & 0x10 ? '1' : '0'),
           (f->dados.bytes[i] & 0x08 ? '1' : '0'),
           (f->dados.bytes[i] & 0x04 ? '1' : '0'),
           (f->dados.bytes[i] & 0x02 ? '1' : '0'),
           (f->dados.bytes[i] & 0x01 ? '1' : '0'));
  }
  printf("|");
  printf("%c%c%c%c%c%c%c%c",
         (f->paridade & 0x80 ? '1' : '0'),
         (f->paridade & 0x40 ? '1' : '0'),
         (f->paridade & 0x20 ? '1' : '0'),
         (f->paridade & 0x10 ? '1' : '0'),
         (f->paridade & 0x08 ? '1' : '0'),
         (f->paridade & 0x04 ? '1' : '0'),
         (f->paridade & 0x02 ? '1' : '0'),
         (f->paridade & 0x01 ? '1' : '0'));
  printf("|\n");
  for(int i = 0; i < 31; i++) {
    printf("%d: %c%c%c%c%c%c%c%c\n", i,
           (f->translate[i] & 0x80 ? '1' : '0'),
           (f->translate[i] & 0x40 ? '1' : '0'),
           (f->translate[i] & 0x20 ? '1' : '0'),
           (f->translate[i] & 0x10 ? '1' : '0'),
           (f->translate[i] & 0x08 ? '1' : '0'),
           (f->translate[i] & 0x04 ? '1' : '0'),
           (f->translate[i] & 0x02 ? '1' : '0'),
           (f->translate[i] & 0x01 ? '1' : '0'));
  }
  printf("\n");
}
