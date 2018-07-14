#include <stdio.h>
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include "myunzip.h"

#define EOCD_SIGNATURE 0x06054b50
#define CDFH_SIGNATURE 0x02014b50

int main(int argc, char** argv) {

  uint32_t possible_sig;
  struct eocd_record eocd;
  struct central_dir_file_header cdfh;
  char *file_name;

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

  fseek(input, eocd.central_dir_offset, SEEK_SET);
  // move to beginning of central directory
  fread(&cdfh, sizeof(struct central_dir_file_header), 1, input);
  // read first file header

  assert(cdfh.signature == CDFH_SIGNATURE);
  file_name = malloc(cdfh.file_name_len + 1);
  fread(file_name, cdfh.file_name_len, 1, input);
  // read the file name
  memset(file_name + cdfh.file_name_len, '\0', 1);

  printf("first file is named: %s\n", file_name);

  fseek(input, cdfh.extra_field_len + cdfh.file_comment_len, SEEK_CUR);
  // move to next cdfh
  fread(&cdfh, sizeof(struct central_dir_file_header), 1, input);
  while (cdfh.signature == CDFH_SIGNATURE) {
    file_name = realloc(file_name, cdfh.file_name_len + 1);
    // reallocate space for the new file name
    fread(file_name, cdfh.file_name_len, 1, input);
    // read the new file name
    memset(file_name + cdfh.file_name_len, '\0', 1);
    // append a null-term
    printf("next file is named: %s\n", file_name);
    fseek(input, cdfh.extra_field_len + cdfh.file_comment_len, SEEK_CUR);
    // move to the next entry
    fread(&cdfh, sizeof(struct central_dir_file_header), 1, input);
    // read the next entry
  }

  free(file_name);
  fclose(input); // don't forget to close the file
  return 0;
  
}

