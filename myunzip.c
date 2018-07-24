#include <stdio.h>
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include "myunzip.h"

#define EOCD_SIGNATURE 0x06054b50
#define CDFH_SIGNATURE 0x02014b50
#define  LFH_SIGNATURE 0x04034b50

int main(int argc, char** argv) {

  uint32_t possible_sig;
  struct eocd_record eocd;
  struct central_dir_file_header cdfh;

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
  
  while (possible_sig != EOCD_SIGNATURE) {
    fseek(input, -5, SEEK_CUR);
    // move back 5 bytes (length of signature plus one)
    fread(&possible_sig, 4, 1, input);
  }

  fseek(input, -4, SEEK_CUR);
  fread(&eocd, sizeof(struct eocd_record), 1, input);

  assert(eocd.signature == EOCD_SIGNATURE);
  // make sure we've got the right thing

  list_contents(input, eocd.central_dir_offset);

  // decode first file
  fseek(input, eocd.central_dir_offset, SEEK_SET);
  fread(&cdfh, sizeof(struct central_dir_file_header), 1, input);
  assert(cdfh.signature == CDFH_SIGNATURE);
  output_deflate(input, cdfh.local_file_header_offset);

  fclose(input); // don't forget to close the file
  return 0;
  
}

void list_contents(FILE *input, int offset) {
  /* list the contents of the archive given a file pointer
   *  and offset to the beginning of the central directory
   */
  char *file_name, *date, *time;
  struct central_dir_file_header cdfh;
  date = malloc(11);
  time = malloc(9);

  printf("\n");
  printf("%9s  %10s %8s  %-s \n", "length", "date", "time", "name");
  printf("---------  ---------- --------  ---- \n");
  
  fseek(input, offset, SEEK_SET);
  // move to beginning of central directory
  fread(&cdfh, sizeof(struct central_dir_file_header), 1, input);
  // read first file header
  
  assert(cdfh.signature == CDFH_SIGNATURE);
  file_name = malloc(cdfh.file_name_len + 1);
  fread(file_name, cdfh.file_name_len, 1, input);
  // read the file name
  file_name[cdfh.file_name_len] = '\0';

  dos_date(date, cdfh.last_modification_date);
  dos_time(time, cdfh.last_modification_time);
  
  printf("%9u  %10s %8s  %-s \n",
         cdfh.uncompressed_size, date, time, file_name);

  fseek(input, cdfh.extra_field_len + cdfh.file_comment_len, SEEK_CUR);
  // move to next cdfh
  fread(&cdfh, sizeof(struct central_dir_file_header), 1, input);
  while (cdfh.signature == CDFH_SIGNATURE) {
    free(file_name);
    file_name = malloc(cdfh.file_name_len + 1);
    // reallocate space for the new file name
    fread(file_name, cdfh.file_name_len, 1, input);
    // read the new file name
    file_name[cdfh.file_name_len] = '\0';
    // append a null-term

    dos_date(date, cdfh.last_modification_date);
    dos_time(time, cdfh.last_modification_time);
  
    printf("%9u  %10s %8s  %-s \n",
           cdfh.uncompressed_size, date, time, file_name);

    fseek(input, cdfh.extra_field_len + cdfh.file_comment_len, SEEK_CUR);
    // move to the next entry
    fread(&cdfh, sizeof(struct central_dir_file_header), 1, input);
    // read the next entry
  }

  free(file_name);
  free(time);
  free(date);

}

void dos_date(char *date_string, uint16_t dos_date) {
  int day  = 0x001f & dos_date; // first five bits
  int mon  = 0x000f & (dos_date >> 5); // bits 5 - 8
  int year = (0x007f & (dos_date >> 9)) + 1980; // bits 9 - 15
  date_string[0] = (mon / 10) + '0';
  date_string[1] = (mon % 10) + '0';
  date_string[2] = '-';
  date_string[3] = (day / 10) + '0';
  date_string[4] = (day % 10) + '0';
  date_string[5] = '-';
  date_string[6] = (year / 1000) + '0';
  date_string[7] = ((year % 1000) / 100) + '0';
  date_string[8] = ((year % 100) / 10) + '0';
  date_string[9] = (year % 10) + '0';
  date_string[10] = '\0';
  
}

void dos_time(char *time_string, uint16_t dos_time) {
  int second = 0x001f & dos_time;
  int minute = (0x07e0 & dos_time) >> 5;
  int hour   = (0xf800 & dos_time) >> 11;
  time_string[0] = (hour / 10) + '0';
  time_string[1] = (hour % 10) + '0';
  time_string[2] = ':';
  time_string[3] = (minute / 10) + '0';
  time_string[4] = (minute % 10) + '0';
  time_string[5] = ':';
  time_string[6] = (second / 10) + '0';
  time_string[7] = (second % 10) + '0';
  time_string[8] = '\0';
}

void output_deflate(FILE *input, int lfh_offset) {
  FILE *output;
  struct local_file_header lfh;
  void *data;
  char *filename;

  // move to the given offset
  fseek(input, lfh_offset, SEEK_SET);
  // read our local file header
  fread(&lfh, sizeof(struct local_file_header), 1, input);
  // check things are right
  assert(lfh.signature == LFH_SIGNATURE);

  // grab memory to store our filename
  filename = malloc(lfh.file_name_len + 4);
  // read the filename
  fread(filename, lfh.file_name_len, 1, input);
  // add our file extension and a null terminator for good measure
  strcpy(filename + lfh.file_name_len, ".df\0");

  // open a destination file pointer
  output = fopen(filename, "wb");

  // allocate memory for the deflate stream
  data = malloc(lfh.compressed_size);
  /* find the deflate stream (we've already read
     the entire header and the filename) */
  fseek(input, lfh.extra_field_len, SEEK_CUR);
  // read the deflate stream
  fread(data, lfh.compressed_size, 1, input);

  // write the deflate stream
  fwrite(data, lfh.compressed_size, 1, output);

  //close the file, free our memory
  fclose(output);
  free(filename);
  free(data);
}
