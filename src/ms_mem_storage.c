//
//  ms_mem_storage.c
//  libms
//
//  Created by Jianguo Wu on 2018/11/21.
//  Copyright © 2018 wujianguo. All rights reserved.
//

#include "ms_mem_storage.h"
#include "ms_memory_pool.h"
#include <math.h>

#define MAX_SIZE_PER_SESSION (20*1024*1024)

static int block_num(uint64_t filesize) {
  return ceil(1.0 * filesize / MS_BLOCK_UNIT_SIZE);
}

static int block_index(uint64_t pos) {
  return (int)(pos / MS_BLOCK_UNIT_SIZE);
}

static int piece_index(uint64_t pos) {
  return (int)(pos % MS_BLOCK_UNIT_SIZE / MS_PIECE_UNIT_SIZE);
}

static struct ms_block *malloc_block() {
  struct ms_block *block = (struct ms_block *)MS_MALLOC(sizeof(struct ms_block));
  memset(block, 0, sizeof(struct ms_block));
  return block;
}

static struct ms_mem_storage *cast_from(struct ms_istorage *st) {
  return (struct ms_mem_storage *)st;
}

static int bit_num(uint64_t filesize) {
  return ceil(1.0 * ceil(1.0 * filesize / MS_PIECE_UNIT_SIZE) / 4);
}

/*
static void print_bitfield(struct ms_istorage *st) {
  MS_ASSERT(MS_PIECE_NUM_OF_PER_BLOCK % 4 == 0);
  struct ms_mem_storage *mem_st = cast_from(st);

  int bitnum = bit_num(st->get_filesize(st));
  int merge_time = 1;
//  while (bitnum > 128 && merge_time < MS_PIECE_NUM_OF_PER_BLOCK / (4 * 4 * 2)) {
//    bitnum /= 2;
//    merge_time *= 2;
//  }
  
  char *bitmap = (char *)MS_MALLOC(bitnum + 1);
  memset(bitmap, 0, bitnum + 1);
  int index_for_bitmap = 0;

  int num = block_num(mem_st->estimate_size);
  int index_for_block = 0;
  for (; index_for_block < num; ++index_for_block) {
    struct ms_block *block = mem_st->blocks[index_for_block];
    if (!block) {
      int inner_index = 0;
      for (; inner_index < MS_PIECE_NUM_OF_PER_BLOCK / (4 * merge_time); ++inner_index) {
        bitmap[index_for_bitmap++] = '0';
        if (index_for_bitmap >= bitnum) {
          break;
        }
      }
      continue;
    }

    int index_for_piece = 0;
    int piece_bit = 0;
    int index_for_byte = 0;
    for (; index_for_piece < MS_PIECE_NUM_OF_PER_BLOCK;) {
      
      int merge_index = 0;
      while (merge_index < merge_time) {
        struct ms_piece *piece = &block->pieces[index_for_piece];
        if (!piece->buf) {
          break;
        }
        index_for_piece++;
        merge_index++;
      }
      if (merge_index == merge_time) {
        piece_bit |= 1 << (4 - index_for_byte - 1);
      }
      
      while (merge_index < merge_time) {
        index_for_piece++;
        merge_index++;
      }
      
      index_for_byte++;
      index_for_byte %= 4;
      if (index_for_byte == 0) {
        static char hex_map[16] = "0123456789ABCDEF";
        bitmap[index_for_bitmap++] = hex_map[piece_bit];
        piece_bit = 0;
        if (index_for_bitmap >= bitnum) {
          break;
        }
      }
    }
  }
  MS_DBG("mem_st:%p %s", mem_st, bitmap);
  MS_FREE(bitmap);
}
*/
static char *get_bitmap(struct ms_istorage *st) {
  struct ms_mem_storage *mem_st = cast_from(st);
  if (st->get_filesize(st) == 0) {
    return mem_st->bitmap;
  }

  int bitnum = bit_num(st->get_filesize(st));
  int merge_time = 1;
  while (bitnum > 128 && merge_time < MS_PIECE_NUM_OF_PER_BLOCK / (4 * 4 * 2)) {
    bitnum /= 2;
    merge_time *= 2;
  }

  if (!mem_st->bitmap) {
    mem_st->bitmap = (char *)MS_MALLOC(bitnum + 1);
    memset(mem_st->bitmap, 0, bitnum + 1);
  }
  MS_ASSERT(MS_PIECE_NUM_OF_PER_BLOCK % 4 == 0);
  
//  int bitnum = bit_num(st->get_filesize(st));
  char *bitmap = mem_st->bitmap;
  memset(bitmap, 0, bitnum + 1);
  int index_for_bitmap = 0;
  
  int num = block_num(mem_st->estimate_size);
  int index_for_block = 0;
  for (; index_for_block < num; ++index_for_block) {
    struct ms_block *block = mem_st->blocks[index_for_block];
    if (!block) {
      int inner_index = 0;
      for (; inner_index < MS_PIECE_NUM_OF_PER_BLOCK / (4 * merge_time); ++inner_index) {
        bitmap[index_for_bitmap++] = '0';
        if (index_for_bitmap >= bitnum) {
          break;
        }
      }
      continue;
    }
    
    int index_for_piece = 0;
    int piece_bit = 0;
    int index_for_byte = 0;
    for (; index_for_piece < MS_PIECE_NUM_OF_PER_BLOCK;) {
      
      int merge_index = 0;
      while (merge_index < merge_time) {
        struct ms_piece *piece = &block->pieces[index_for_piece];
        if (!piece->buf) {
          break;
        }
        index_for_piece++;
        merge_index++;
      }
      if (merge_index == merge_time) {
        piece_bit |= 1 << (4 - index_for_byte - 1);
      }
      
      while (merge_index < merge_time) {
        index_for_piece++;
        merge_index++;
      }
      
      index_for_byte++;
      index_for_byte %= 4;
      if (index_for_byte == 0) {
        static char hex_map[16] = "0123456789ABCDEF";
        bitmap[index_for_bitmap++] = hex_map[piece_bit];
        piece_bit = 0;
        if (index_for_bitmap >= bitnum) {
          break;
        }
      }
    }
  }
//  MS_DBG("mem_st:%p %s", mem_st, bitmap);
//  MS_FREE(bitmap);

  return mem_st->bitmap;
}

//static void update_bitmap(char *bitmap, int block_index, int piece_index) {
//  int bit_piece_index = block_index * MS_PIECE_NUM_OF_PER_BLOCK / 4 + piece_index;
//
//}

static int64_t get_filesize(struct ms_istorage *st) {
  struct ms_mem_storage *mem_st = cast_from(st);
  return mem_st->filesize;
}

static void realloc_blocks(struct ms_mem_storage *mem_st, int64_t new_size) {
  int64_t old_size = mem_st->estimate_size;
  if (mem_st->filesize > 0) {
    old_size = mem_st->filesize;
  }
  mem_st->blocks = MS_REALLOC(mem_st->blocks, block_num(new_size) * sizeof(struct ms_block *));
  memset((char *)mem_st->blocks + block_num(old_size) * sizeof(struct ms_block *), 0, (block_num(new_size) - block_num(old_size)) * sizeof(struct ms_block *));
  
}

static void set_filesize(struct ms_istorage *st, int64_t filesize) {
  struct ms_mem_storage *mem_st = cast_from(st);
  MS_ASSERT(filesize > 0);
  MS_ASSERT(filesize >= mem_st->filesize);
  
  realloc_blocks(mem_st, filesize);
  
  mem_st->filesize = filesize;
  mem_st->estimate_size = filesize;
}

static int64_t get_estimate_size(struct ms_istorage *st) {
  struct ms_mem_storage *mem_st = cast_from(st);
  return mem_st->estimate_size;
}

static void set_content_size(struct ms_istorage *st, int64_t from, int64_t size) {
  struct ms_mem_storage *mem_st = cast_from(st);
  if (mem_st->filesize > 0) {
    return;
  }
  if (from + size > mem_st->estimate_size) {
    realloc_blocks(mem_st, from + size);
    mem_st->estimate_size = from + size;
  }
}

static int64_t get_completed_size(struct ms_istorage *st) {
  struct ms_mem_storage *mem_st = cast_from(st);
  return mem_st->completed_size;
}

static int64_t max_cache_len(struct ms_istorage *st) {
  return 20*1024*1024;
}

static int64_t max_cache_left(struct ms_istorage *st) {
  return 10*1024*1024;
}

static int64_t hold_head_size(struct ms_istorage *st) {
  return 2*1024*1024;
}

static int64_t hold_size(struct ms_istorage *st, int pos_count) {
  return hold_head_size(st) + (max_cache_left(st) + max_cache_len(st)) * pos_count;
}

static void hold_range_from(struct ms_istorage *st, int64_t pos, int64_t *from, int64_t *end) {
  int64_t left = max_cache_left(st);
  if (pos < left) {
    *from = 0;
  } else {
    *from = pos - left;
  }
  *end = pos + max_cache_len(st);
}

static int should_hold_block(struct ms_istorage *st, int index_for_block, int64_t *hold_pos, int hold_pos_len) {
  int i = 0;
  for (; i < hold_pos_len; ++i) {
    int64_t hold_from, hold_end;
    hold_range_from(st, hold_pos[i], &hold_from, &hold_end);
    int64_t block_start = index_for_block * MS_BLOCK_UNIT_SIZE;
    int64_t block_end = block_start + MS_BLOCK_UNIT_SIZE;
    if (hold_from <= block_start && block_end <= hold_end) {
      return 1;
    }
  }
  return 0;
}

static int should_hold_piece(struct ms_istorage *st, int index_for_block, int index_for_piece, int64_t *hold_pos, int hold_pos_len) {
  int i = 0;
  for (; i < hold_pos_len; ++i) {
    int64_t hold_from, hold_end;
    hold_range_from(st, hold_pos[i], &hold_from, &hold_end);
    int64_t piece_start = index_for_block * MS_BLOCK_UNIT_SIZE + index_for_piece * MS_PIECE_UNIT_SIZE;
    int64_t piece_end = piece_start + MS_PIECE_UNIT_SIZE;
    if (hold_from <= piece_start && piece_end <= hold_end) {
      return 1;
    }
  }
  return 0;
}

static size_t piece_len(struct ms_istorage *st, int index_for_block, int index_for_piece) {
  int64_t start = index_for_block * MS_BLOCK_UNIT_SIZE + index_for_piece * MS_PIECE_UNIT_SIZE;
  int64_t filesize = st->get_filesize(st);
  if (start + MS_PIECE_UNIT_SIZE > filesize) {
    return filesize - start;
  } else {
    return MS_PIECE_UNIT_SIZE;
  }
}

static void clear_buffer_for(struct ms_istorage *st, int64_t pos, size_t len, int64_t *hold_pos, int hold_pos_len) {
  struct ms_mem_storage *mem_st = cast_from(st);
  if (mem_st->completed_size + (int64_t)len <= hold_size(st, hold_pos_len)) {
    return;
  }
  
//  int index = 0;
//  for (; index < hold_pos_len; ++index) {
//    MS_DBG("%"INT64_FMT, hold_pos[index]);
//  }
  
  int release_len = (int)len;
  int64_t from = hold_head_size(st);
  int index_for_block = block_index(from);
  int index_for_piece = piece_index(from);
  
  int num = block_num(mem_st->estimate_size);
  for (; index_for_block < num; ++index_for_block) {
    struct ms_block *block = mem_st->blocks[index_for_block];
    if (!block) {
      continue;
    }
    if (should_hold_block(st, index_for_block, hold_pos, hold_pos_len)) {
      continue;
    }
    index_for_piece = 0;
    int buf_count = 0;
    int release_count = 0;
    for (; index_for_piece < MS_PIECE_NUM_OF_PER_BLOCK; ++index_for_piece) {
      struct ms_piece *piece = &block->pieces[index_for_piece];
      if (!piece->buf) {
        continue;
      }
      buf_count += 1;
      if (should_hold_piece(st, index_for_block, index_for_piece, hold_pos, hold_pos_len)) {
        continue;
      }
      release_count += 1;
//      MS_DBG("release block %d, piece %d, start pos %d", index_for_block, index_for_piece, index_for_block * MS_BLOCK_UNIT_SIZE + index_for_piece * MS_PIECE_UNIT_SIZE);
      ms_free_piece_buf(piece->buf);
      piece->buf = (char *)0;
      mem_st->completed_size -= piece_len(st, index_for_block, index_for_piece);
      release_len -= piece_len(st, index_for_block, index_for_piece);
    }
    if (release_count > 0 && release_count == buf_count) {
      MS_FREE(block);
      mem_st->blocks[index_for_block] = (struct ms_block *)0;
    }
    if (release_len <= 0) {
      return;
    }
  }
}

static void cached_from(struct ms_istorage *st, int64_t from, int64_t *pos, int64_t *len) {
  *pos = 0;
  *len = 0;
  struct ms_mem_storage *mem_st = cast_from(st);
  
  if (mem_st->estimate_size == 0) {
    *pos = from;
    *len = -1;
    return;
  }
  
  int index_for_block = block_index(from);
  int index_for_piece = piece_index(from);

  int num = block_num(mem_st->estimate_size);
  int find_first = 0;
  for (; index_for_block < num; ++index_for_block) {
    struct ms_block *block = mem_st->blocks[index_for_block];
    if (!block) {
      if (!find_first) {
        continue;
      }
      MS_ASSERT(*pos + *len <= mem_st->estimate_size);
      return;
    }
    
    if (index_for_block > block_index(from)) {
      index_for_piece = 0;
    }
    
    for (; index_for_piece < MS_PIECE_NUM_OF_PER_BLOCK; ++index_for_piece) {
      struct ms_piece *piece = &block->pieces[index_for_piece];
      if (!piece->buf) {
        if (!find_first) {
          continue;
        }
        MS_ASSERT(*pos <= mem_st->estimate_size);
        if (*pos + *len > mem_st->estimate_size) {
          *len = mem_st->estimate_size - *pos;
        }
        return;
      }
      if (!find_first) {
        *pos = MS_BLOCK_UNIT_SIZE * index_for_block + MS_PIECE_UNIT_SIZE * index_for_piece;
        find_first = 1;
        *len = MS_PIECE_UNIT_SIZE;
        if (*pos < from) {
          *len = MS_PIECE_UNIT_SIZE - (from - *pos);
          *pos = from;
        }
      } else {
        *len += MS_PIECE_UNIT_SIZE;
      }
    }
  }
  
  MS_ASSERT(*pos <= mem_st->estimate_size);
  if (*pos + *len > mem_st->estimate_size) {
    *len = mem_st->estimate_size - *pos;
  }
  
}

static size_t storage_write(struct ms_istorage *st, const char *buf, int64_t pos, size_t len) {
  struct ms_mem_storage *mem_st = cast_from(st);
  MS_ASSERT(pos % MS_PIECE_UNIT_SIZE == 0);
  MS_ASSERT(pos + (int64_t)len == mem_st->estimate_size || len % MS_PIECE_UNIT_SIZE == 0);
  if (pos + (int64_t)len == mem_st->estimate_size) {
    MS_DBG("complete");
  }
  size_t write = 0;
  int index_for_block = block_index(pos);
  while (write < len) {
    struct ms_block *block = mem_st->blocks[index_for_block];
    if (!block) {
      block = malloc_block();
      mem_st->blocks[index_for_block] = block;
    }
    int index_for_piece = piece_index(pos + write);
    while (write < len) {
      struct ms_piece *piece = &block->pieces[index_for_piece];
      size_t write_len = MS_PIECE_UNIT_SIZE;
      if (write_len > len - write) {
        write_len = len - write;
      }
      if (!piece->buf) {
        piece->buf = ms_malloc_piece_buf();
        mem_st->completed_size += write_len;
      }
      memcpy(piece->buf, buf + write, write_len);
      write += write_len;
      
      index_for_piece += 1;
      if (index_for_piece >= MS_PIECE_NUM_OF_PER_BLOCK) {
        break;
      }
    }
    
    index_for_block += 1;
    if (index_for_block >= block_num(mem_st->estimate_size)) {
      break;
    }
  }
//  MS_DBG("%lld, %zu, write: %zu", pos, len, write);
//  print_bitfield(st);
//  mem_st->completed_size += write;
  return write;
}

static size_t storage_read(struct ms_istorage *st, char *buf, int64_t pos, size_t len) {
  struct ms_mem_storage *mem_st = cast_from(st);
  MS_ASSERT(pos >= 0);
  MS_ASSERT(mem_st->estimate_size == 0 || pos + (int64_t)len <= mem_st->estimate_size);
  if (mem_st->estimate_size == 0 && mem_st->filesize == 0) {
    return 0;
  }
  size_t read = 0;
  int index_for_block = block_index(pos);
  while (read < len) {
    struct ms_block *block = mem_st->blocks[index_for_block];
    if (!block) {
      break;
    }
    int index_for_piece = piece_index(pos+read);
    while (read < len) {
      struct ms_piece *piece = &block->pieces[index_for_piece];
      if (!piece->buf) {
        //                MS_DBG("%lld, %zu, read: %zu", pos, len, read);
        return read;
      }
      int offset = (pos + read) % MS_PIECE_UNIT_SIZE;
      size_t read_len = MS_PIECE_UNIT_SIZE - offset;
      if (read_len > len - read) {
        read_len = len - read;
      }
      memcpy(buf + read, piece->buf + offset, read_len);
      read += read_len;
      
      index_for_piece += 1;
      if (index_for_piece >= MS_PIECE_NUM_OF_PER_BLOCK) {
        break;
      }
    }
    
    index_for_block += 1;
    if (block_num(mem_st->estimate_size) <= index_for_block) {
      break;
    }
  }
  //    MS_DBG("%lld, %zu, read: %zu", pos, len, read);
  return read;
}

static void storage_close(struct ms_istorage *st) {
  struct ms_mem_storage *mem_st = cast_from(st);
  MS_DBG("mem_st:%p", mem_st);
  
  int index_for_block = 0;
  int num = block_num(mem_st->estimate_size);
  for (; index_for_block < num; ++index_for_block) {
    struct ms_block *block = mem_st->blocks[index_for_block];
    if (!block) {
      continue;
    }
    int index_for_piece = 0;
    for (; index_for_piece < MS_PIECE_NUM_OF_PER_BLOCK; ++index_for_piece) {
      struct ms_piece *piece = &block->pieces[index_for_piece];
      if (piece->buf) {
        ms_free_piece_buf(piece->buf);
        piece->buf = (char *)0;
      }
    }
    MS_FREE(block);
  }
  MS_FREE(mem_st->blocks);
  MS_FREE(mem_st);
}


struct ms_mem_storage *ms_mem_storage_open() {
  struct ms_mem_storage *mem_st = (struct ms_mem_storage *)MS_MALLOC(sizeof(struct ms_mem_storage));
  memset(mem_st, 0, sizeof(struct ms_mem_storage));
  
  mem_st->st.get_filesize = get_filesize;
  mem_st->st.set_filesize = set_filesize;
  mem_st->st.get_estimate_size = get_estimate_size;
  mem_st->st.set_content_size = set_content_size;
  mem_st->st.get_completed_size = get_completed_size;
  mem_st->st.cached_from = cached_from;
  mem_st->st.write = storage_write;
  mem_st->st.read = storage_read;
  mem_st->st.close = storage_close;
  mem_st->st.get_bitmap = get_bitmap;
  mem_st->st.max_cache_len = max_cache_len;
  mem_st->st.clear_buffer_for = clear_buffer_for;
  
  MS_DBG("mem_st:%p", mem_st);
  return mem_st;
}

