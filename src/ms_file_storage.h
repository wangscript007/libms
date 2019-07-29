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


struct ms_file_storage {
  struct ms_istorage st;
//  int64_t filesize;
//  int64_t completed_size;
  int     fd;
  
  char    header[4];  // "lbms", len: 4
  int     version;    //  len: 4
  int     url_len;    //  len: 4
  char    *url;       //  len: url_len
  int64_t filesize;   //  len: 8
  int64_t completed_size;  // len: 8
  int     block_unit_size; // MS_BLOCK_UNIT_SIZE
  char    *block_bitmap;   // ceil(1.0 * filesize / block_unit_size)
  int     *physical_indexes; // ceil(1.0 * filesize / block_unit_size)

  
//  struct ms_mem_storage *mem_st;
};

struct ms_file_storage *ms_file_storage_open(const char *url, const char *path);

#endif /* ms_file_storage_h */

