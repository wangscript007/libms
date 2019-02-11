//
//  ms_file_storage.h
//  libms
//
//  Created by Jianguo Wu on 2018/12/5.
//  Copyright © 2018 wujianguo. All rights reserved.
//

#ifndef ms_file_storage_h
#define ms_file_storage_h

#include "ms_server.h"
#include "ms_memory_pool.h"
#include "ms_mem_storage.h"

struct ms_ct_block {
  int   logical_index;
  int   physical_index;
  char  *bitmap;
};

// control file
struct ms_ct_file {
  int     fd;
  
  char    header[4];  // "lbms"
  int     version;
  int     url_len;
  char    *url;
  int64_t filesize;
  int64_t downloadsize;
  int     block_unit_size; // MS_BLOCK_UNIT_SIZE
  char    *block_bitmap;   // ceil(1.0 * filesize / block_unit_size)
  int     *physical_indexes; // ceil(1.0 * filesize / block_unit_size)
  
  int     in_flight_block_num;
  struct ms_ct_block  **in_flight_blocks;
};

struct ms_file_storage {
  struct ms_istorage st;
  int64_t filesize;
  int     fd;
  
  struct ms_mem_storage *mem_st;
  struct ms_ct_file     *ct_file;
};

struct ms_file_storage *ms_file_storage_open(const char *url, const char *path);

#endif /* ms_file_storage_h */

