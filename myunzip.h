#ifndef MYUNZIP_H
#define MYUNZIP_H

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

struct eocd_record {

  uint32_t signature;
  uint16_t disk_num;
  uint16_t central_dir_start_disk;
  uint16_t num_central_dir_on_disk;
  uint16_t num_central_dir_total;
  uint32_t central_dir_size;
  uint32_t central_dir_offset;
  uint16_t comment_len;
  
};

struct __attribute__((__packed__)) central_dir_file_header {

  uint32_t signature;
  uint16_t created_version;
  uint16_t needed_version;
  uint16_t flag;
  uint16_t compression_method;
  uint16_t last_modification_time;
  uint16_t last_modification_date;
  uint32_t crc32;
  uint32_t compressed_size;
  uint32_t uncompressed_size;
  uint16_t file_name_len;
  uint16_t extra_field_len;
  uint16_t file_comment_len;
  uint16_t file_start_disk_num;
  uint16_t internal_file_attr;
  uint32_t external_file_attr;
  uint32_t local_file_header_offset;
  
};

#endif
