//
//  ms_task_manager.c
//  ms
//
//  Created by Jianguo Wu on 2019/5/24.
//  Copyright © 2019 libms. All rights reserved.
//

#include "ms_task_manager.h"
#include "ms_http_pipe.h"
#include "ms_mem_storage.h"

static struct ms_ipipe *open_pipe(const struct mg_str url,
                                  int64_t pos,
                                  int64_t len,
                                  struct ms_ipipe_callback callback) {
  return &ms_http_pipe_create(url, pos, len, callback)->pipe;
}

static struct ms_istorage *open_storage(void) {
  return &ms_mem_storage_open()->st;
}

void ms_remove_task_if_need(struct ms_server *server, double ts) {
  QUEUE *q = QUEUE_NEXT(&server->tasks);
  struct ms_task *task = NULL;
  while (q != &server->tasks) {
    task = QUEUE_DATA(q, struct ms_task, node);
    q = QUEUE_NEXT(q);
    if (QUEUE_EMPTY(&task->readers)) {
      if (mg_time() - task->close_ts >= ts) {
        task->task.close(&task->task);
      }
    }
  }
}

struct ms_task *ms_find_or_create_task(const char *url, struct ms_server *server) {
  QUEUE *q;
  struct ms_task *task = NULL;
  QUEUE_FOREACH(q, &server->tasks) {
    task = QUEUE_DATA(q, struct ms_task, node);
    if (mg_strcmp(task->url, mg_mk_str(url)) == 0) {
      return task;
    }
  }
  ms_remove_task_if_need(server, 5);
  struct ms_factory factory = {
    open_storage,
    open_pipe
  };
  task = ms_task_open(mg_mk_str(url), factory);
  QUEUE_INSERT_TAIL(&server->tasks, &task->node);
  return task;
}

