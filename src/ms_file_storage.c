//
//  ms_file_storage.c
//  libms
//
//  Created by Jianguo Wu on 2018/12/5.
//  Copyright © 2018 wujianguo. All rights reserved.
//

#include "ms_file_storage.h"

/*
// file io system
struct ms_file_io_open_op {
  char  *file_path;
  int   flags;
  int   mode;
};

struct ms_file_io_write_op {
  int     fd;
  char    *buf;
  int64_t pos;
  size_t len;
};

struct ms_file_io_read_op {
  int     fd;
  char    *buf;
  int64_t pos;
  size_t len;
};

struct ms_file_io_close_op {
  int   fd;
};

struct ms_file_io_sync_op {
  int   fd;
};


enum ms_file_op_type {
  ms_file_op_type_open,
  ms_file_op_type_write,
  ms_file_op_type_read,
  ms_file_op_type_close,
  ms_file_op_type_sync
};

struct ms_file_io_op {
  enum ms_file_op_type op_type;
  void *op;
  void *data;
};


struct file_io_callback {
  void (*on_open)(struct ms_file_io_op *op, int ret, int fd);
  void (*on_write)(struct ms_file_io_op *op, int ret, size_t write);
  void (*on_read)(struct ms_file_io_op *op, int ret, size_t read);
  void (*on_close)(struct ms_file_io_op *op, int ret);
  void (*on_sync)(struct ms_file_io_op *op, int ret);
};


static void post_message(struct ms_file_io_op *op, struct file_io_callback callback) {
  switch (op->op_type) {
    case ms_file_op_type_open:
      
      break;
    case ms_file_op_type_write:
      break;
    case ms_file_op_type_read:
      break;
    case ms_file_op_type_close:
      break;
    case ms_file_op_type_sync:
      break;
    default:
      break;
  }
}



static void on_open(struct ms_file_io_op *op, int ret, int fd) {
  
}

static void on_write(struct ms_file_io_op *op, int ret, size_t write) {
  
}

static void on_read(struct ms_file_io_op *op, int ret, size_t read) {
  
}

static void on_close(struct ms_file_io_op *op, int ret) {
  
}

static void on_sync(struct ms_file_io_op *op, int ret) {
  
}

static struct file_io_callback g_callback = {
  on_open,
  on_write,
  on_read,
  on_close,
  on_sync
};
*/

/*
static void read_header(struct ms_file_storage *file_st) {
  MS_ASSERT(file_st->fd > 0);
  struct ms_file_storage *ct = file_st;
  char header[4] = {0};
  pread(file_st->fd, header, 4, 0);

}

static void write_base_header(struct ms_file_storage *file_st) {
  MS_ASSERT(file_st->fd > 0);
  struct ms_file_storage *ct = file_st;
  int header_len = 0;
  header_len += sizeof(ct->header);
  header_len += sizeof(ct->version);
  header_len += sizeof(ct->url_len);
  header_len += ct->url_len;
  header_len += sizeof(ct->filesize);
  header_len += sizeof(ct->completed_size);
  header_len += sizeof(ct->block_unit_size);
  
  char *header = MS_MALLOC(header_len);
  memset(header, 0, header_len);
  memcpy(header, ct->header, sizeof(ct->header));
  header += sizeof(ct->header);
  
  uint32_t version = htonl(ct->version);
  memcpy(header, &version, sizeof(version));
  header += sizeof(version);
  
  uint32_t url_len = htonl(ct->url_len);
  memcpy(header, &url_len, sizeof(url_len));
  header += sizeof(url_len);
  
  memcpy(header, ct->url, ct->url_len);
  header += ct->url_len;
  
  uint64_t filesize = htonll(ct->filesize);
  memcpy(header, &filesize, sizeof(filesize));
  header += sizeof(filesize);

  uint64_t completed_size = htonll(ct->completed_size);
  memcpy(header, &completed_size, sizeof(completed_size));
  header += sizeof(completed_size);

  uint32_t block_unit_size = htonl(ct->block_unit_size);
  memcpy(header, &block_unit_size, sizeof(block_unit_size));
  header += sizeof(block_unit_size);

  pwrite(file_st->fd, header, header_len, 0);
}
*/

static struct ms_file_storage *cast_from(struct ms_istorage *st) {
  return (struct ms_file_storage *)st;
}

static int64_t get_filesize(struct ms_istorage *st) {
  struct ms_file_storage *file_st = cast_from(st);
  return file_st->filesize;
}

static void set_filesize(struct ms_istorage *st, int64_t filesize) {
  struct ms_file_storage *file_st = cast_from(st);
  file_st->filesize = filesize;
//  file_st->mem_st->st.set_filesize(&file_st->mem_st->st, filesize);
}

static int64_t get_estimate_size(struct ms_istorage *st) {
  struct ms_file_storage *file_st = cast_from(st);
  return file_st->filesize;
}

static int64_t max_cache_len(struct ms_istorage *st) {
  struct ms_file_storage *file_st = cast_from(st);
  if (file_st->filesize == 0) {
    return 1024*1024*1024;
  } else {
    return file_st->filesize;
  }
}

//static int should_clear_buffer_for(struct ms_istorage *st, int64_t pos, size_t len, int64_t *hold_pos, int hold_pos_len) {
//  return 1;
//}

static void clear_buffer_for(struct ms_istorage *st, int64_t pos, size_t len, int64_t *hold_pos, int hold_pos_len) {
//  if (!should_clear_buffer_for(st, pos, len, hold_pos, hold_pos_len)) {
//    return;
//  }
//  struct ms_file_storage *file_st = cast_from(st);
//  file_st->mem_st->st.clear_buffer_for(&file_st->mem_st->st, pos, len, hold_pos, hold_pos_len);
}

static void set_content_size(struct ms_istorage *st, int64_t from, int64_t size) {
//  struct ms_file_storage *file_st = cast_from(st);
//  file_st->mem_st->st.set_content_size(&file_st->mem_st->st, from, size);
}

static int64_t get_completed_size(struct ms_istorage *st) {
  struct ms_file_storage *file_st = cast_from(st);
  return file_st->completed_size;
}

static char *get_bitmap(struct ms_istorage *st) {
  return "";
}

static void wanted_pos_from(struct ms_istorage *st, int64_t from, int64_t *pos, int64_t *len) {
  *len = 0;
}

static size_t storage_write(struct ms_istorage *st, const char *buf, int64_t pos, size_t len) {
  struct ms_file_storage *file_st = cast_from(st);
  return pwrite(file_st->fd, buf, pos, len);
}

static size_t storage_read(struct ms_istorage *st, char *buf, int64_t pos, size_t len) {
  struct ms_file_storage *file_st = cast_from(st);
  return pread(file_st->fd, buf, len, pos);
}

static void storage_close(struct ms_istorage *st) {
  struct ms_file_storage *file_st = cast_from(st);
  MS_DBG("file_st:%p", file_st);
//  file_st->mem_st->st.close(&file_st->mem_st->st);
  close(file_st->fd);
  MS_FREE(file_st);
}

struct ms_file_storage *ms_file_storage_open(const char *url, const char *path) {
  struct ms_file_storage *file_st = (struct ms_file_storage *)MS_MALLOC(sizeof(struct ms_file_storage));
  memset(file_st, 0, sizeof(struct ms_file_storage));
  
  file_st->fd = open(path, O_RDWR);
  
  file_st->st.get_filesize = get_filesize;
  file_st->st.set_filesize = set_filesize;
  file_st->st.get_estimate_size = get_estimate_size;
  file_st->st.set_content_size = set_content_size;
  file_st->st.get_completed_size = get_completed_size;
  file_st->st.cached_from = wanted_pos_from;
  file_st->st.write = storage_write;
  file_st->st.read = storage_read;
  file_st->st.close = storage_close;
  file_st->st.get_bitmap = get_bitmap;
  file_st->st.max_cache_len = max_cache_len;
  file_st->st.clear_buffer_for = clear_buffer_for;

//  file_st->mem_st = ms_mem_storage_open();
  
  MS_DBG("file_st:%p", file_st);
  return file_st;
  
}

