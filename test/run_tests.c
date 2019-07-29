//
//  run_tests.c
//  libms
//
//  Created by Jianguo Wu on 2018/11/28.
//  Copyright © 2018 wujianguo. All rights reserved.
//

#include "run_tests.h"
#include "ms.h"
#include "fake_server.h"

//typedef void (*start_case_func)(notify_case_done_func on_case_done);

typedef enum {
#define XX(i, f)    CASE_INDEX_##i,
  TEST_CASE_MAP(XX)
#undef XX
  TEST_CASE_NUM
} TEST_CASE_INDEXS;

static void run_async_case(int index);

#define GEN_TEST_DONE(i,f) static void on_##f##_done() { run_async_case(CASE_INDEX_##i + 1);}
TEST_CASE_MAP(GEN_TEST_DONE)
#undef GEN_TEST_DONE


static void timer_handler(struct mg_connection *nc, int ev, void *ev_data) {
  if (ev == MG_EV_TIMER) {
    stop_fake_server();
    ms_stop();
  }
}


#define GEN_CASE_INFO_FRAME(i, f) case CASE_INDEX_##i: { test_##f(on_##f##_done); break;}
static void run_async_case(int index)
{
  MS_DBG("%d", index);
  switch (index) {
      TEST_CASE_MAP(GEN_CASE_INFO_FRAME)
      
    default:
    {
      struct mg_connection *nc = mg_add_sock(&ms_default_server()->mgr, INVALID_SOCKET, timer_handler);
      mg_set_timer(nc, mg_time() + 6);
      break;
    }
  }
}
#undef GEN_CASE_INFO_FRAME


static void on_server_start() {
  start_fake_server();
  run_async_case(0);
}


void run_async_tests(const char *path) {
  ms_start(8090, path, on_server_start);
}



typedef enum {
#define XX(i, f)    SYNC_CASE_INDEX_##i,
  SYNC_TEST_CASE_MAP(XX)
#undef XX
  SYNC_TEST_CASE_NUM
} SYNC_TEST_CASE_INDEXS;


#define GEN_SYNC_CASE_INFO_FRAME(i, f) if (index == SYNC_CASE_INDEX_##i) { test_##f();}
static void run_sync_case(int index)
{
  MS_DBG("%d", index);
  while (index < SYNC_TEST_CASE_NUM) {
    SYNC_TEST_CASE_MAP(GEN_SYNC_CASE_INFO_FRAME)
    index += 1;
  }
}
#undef GEN_SYNC_CASE_INFO_FRAME


void run_sync_tests(const char *path) {
  run_sync_case(0);
}




struct ms_test_memory_mem {
  QUEUE   node;
  void    *ptr;
  size_t  size;
  
  char    *trace;
};

struct ms_test_memory_monitor {
  QUEUE mem_list;
};

static struct ms_test_memory_monitor s_memory_monitor;

#include <execinfo.h>
#define BT_BUF_SIZE 100
static char * backtrace_description(void) {
  int j, nptrs;
  void *buffer[BT_BUF_SIZE];
  char **strings;
  
  nptrs = backtrace(buffer, BT_BUF_SIZE);

  strings = backtrace_symbols(buffer, nptrs);
  if (strings == NULL) {
    perror("backtrace_symbols");
    exit(EXIT_FAILURE);
  }
  
  char *ret = malloc(1024);
  memset(ret, 0, 1024);
  for (j = 0; j < nptrs; j++) {
    strcat(ret, strings[j]);
    if (j > 8) {
      break;
    }
  }
  
  free(strings);
  return ret;
}

void *ms_malloc_test(size_t size) {
  void *ptr = malloc(size);
  struct ms_test_memory_mem *mem = (struct ms_test_memory_mem *)malloc(sizeof(struct ms_test_memory_mem));
  memset(mem, 0, sizeof(struct ms_test_memory_mem));
  mem->ptr = ptr;
  mem->size = size;
  mem->trace = backtrace_description();
  QUEUE_INIT(&mem->node);
  QUEUE_INSERT_TAIL(&s_memory_monitor.mem_list, &mem->node);
//  after_tests();
  return ptr;
}

void *ms_realloc_test(void *ptr, size_t size) {
  void *ret = realloc(ptr, size);
  QUEUE *q;
  struct ms_test_memory_mem *mem;
  QUEUE_FOREACH(q, &s_memory_monitor.mem_list) {
    mem = QUEUE_DATA(q, struct ms_test_memory_mem, node);
    if (mem->ptr == ptr) {
      QUEUE_REMOVE(q);
      free(mem);
      break;
    }
  }
  
  mem = (struct ms_test_memory_mem *)malloc(sizeof(struct ms_test_memory_mem));
  memset(mem, 0, sizeof(struct ms_test_memory_mem));
  mem->ptr = ret;
  mem->size = size;
  mem->trace = backtrace_description();
  QUEUE_INIT(&mem->node);
//  QUEUE_INSERT_TAIL(&s_memory_monitor.mem_list, &mem->node);
  QUEUE_INSERT_HEAD(&s_memory_monitor.mem_list, &mem->node);
  
  return ret;
}

void ms_free_test(void *ptr) {
  QUEUE *q;
  struct ms_test_memory_mem *mem;
  QUEUE_FOREACH(q, &s_memory_monitor.mem_list) {
    mem = QUEUE_DATA(q, struct ms_test_memory_mem, node);
    if (mem->ptr == ptr) {
      QUEUE_REMOVE(q);
      free(mem->trace);
      free(mem);
      break;
    }
  }
  free(ptr);
}


void test_setup(void) {
  QUEUE_INIT(&s_memory_monitor.mem_list);
}

void test_tear_down(void) {
  QUEUE *q;
  struct ms_test_memory_mem *mem;
  QUEUE_FOREACH(q, &s_memory_monitor.mem_list) {
    mem = QUEUE_DATA(q, struct ms_test_memory_mem, node);
    MS_DBG("%p, %zu", mem->ptr, mem->size);
    MS_DBG("%s\n", mem->trace);
  }
  MS_ASSERT(QUEUE_EMPTY(&s_memory_monitor.mem_list));
}

