#ifndef UNZIP_H
#define UNZIP_H

#include <inttypes.h>

struct local_file_header {

  uint32_t signature;
  uint16_t version;
  uint16_t bit_flag;
  uint16_t compression_method;
  uint16_t last_modification_time;
  uint16_t last_modification_date;
  uint32_t crc32;
  uint32_t compressed_size;
  uint32_t uncompressed_size;
  uint16_t file_name_len;
  uint16_t extra_field_len;
  
};

#endif
