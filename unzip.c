#include <stdio.h>
#include "unzip.h"

#define EOCD_SIGNATURE 0x06054b50

int main() {

  uint32_t possible_sig;
  FILE *input = fopen("sample.zip", "rb");

  fseek(input, -22, SEEK_END); // comment starts at 22
  fread(&possible_sig, 4, 1, input); // read possible signature
  printf("possible_sig: %#.8x\n",possible_sig);
  while (possible_sig != EOCD_SIGNATURE) {
    fseek(input, -5, SEEK_CUR); // move back 5 bytes (one more than we read before)
    fread(&possible_sig, 4, 1, input);
    printf("possible_sig: %#.8x\n",possible_sig);
  }


  
  fclose(input);
  
}
