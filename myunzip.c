#include <stdio.h>
#include <assert.h>
#include "myunzip.h"

#define EOCD_SIGNATURE 0x06054b50

int main(int argc, char** argv) {

  uint32_t possible_sig;
  struct eocd_record eocd;

  if (argc != 2) {
    // not exactly two arguments
    printf("usage: %s file[.zip]\n",argv[0]);
    return 1;
    // return with error code
  }
  FILE *input = fopen(argv[1], "rb"); // open the argument file
  if (!input) {
    printf("cannot open file %s\n", argv[1]);
    return 1;
  }

  fseek(input, -22, SEEK_END); // comment starts at 22
  fread(&possible_sig, 4, 1, input); // read possible signature
  printf("possible_sig: %#.8x\n",possible_sig);
  
  while (possible_sig != EOCD_SIGNATURE) {
    fseek(input, -5, SEEK_CUR);
    // move back 5 bytes (length of signature plus one)
    fread(&possible_sig, 4, 1, input);
    printf("possible_sig: %#.8x\n",possible_sig);
  }

  fseek(input, -4, SEEK_CUR);
  fread(&eocd, sizeof(struct eocd_record), 1, input);

  assert(eocd.signature == EOCD_SIGNATURE);
  // make sure we've got the right thing

  fclose(input); // don't forget to close the file
  return 0;
  
}
