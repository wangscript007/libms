//
//  ms_preloader.c
//  ms
//
//  Created by Jianguo Wu on 2019/5/24.
//  Copyright © 2019 libms. All rights reserved.
//

#include "ms_preloader.h"
static void on_send(struct ms_ireader *reader, int num_sent_bytes) {

}

static void on_filesize(struct ms_ireader *reader, int64_t filesize) {

}

static void on_content_size_from(struct ms_ireader *reader, int64_t pos, int64_t size) {

}

static void on_recv(struct ms_ireader *reader, int64_t pos, size_t len) {

}

static void on_error(struct ms_ireader *reader, int code) {

}

struct ms_preloader *ms_preloader_open(struct ms_itask *task, int64_t len) {
  struct ms_preloader *preloader = MS_MALLOC(sizeof(struct ms_preloader));
  memset(preloader, 0, sizeof(struct ms_preloader));
  MS_DBG("preloader:%p", preloader);
  QUEUE_INIT(&preloader->node);
  QUEUE_INIT(&preloader->reader.node);
  preloader->task = task;
  preloader->reader.pos = 0;
  preloader->reader.len = len;
  preloader->reader.req_pos = 0;
  preloader->reader.req_len = len;
  preloader->reader.on_send  = on_send;
  preloader->reader.on_recv  = on_recv;
  preloader->reader.on_error = on_error;
  preloader->reader.on_filesize = on_filesize;
  preloader->reader.on_content_size_from = on_content_size_from;
  
  int64_t filesize = task->get_filesize(task);
  if (preloader->reader.len == 0 && filesize > 0) {
    preloader->reader.len = filesize - preloader->reader.pos;
  }
  return preloader;
}

void ms_preloader_close(struct ms_preloader *preloader) {
  MS_DBG("preloader:%p", preloader);
  MS_FREE(preloader);
}
