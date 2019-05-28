//
//  ms_server_handler.c
//  ms
//
//  Created by Jianguo Wu on 2019/5/24.
//  Copyright © 2019 libms. All rights reserved.
//

#include "ms_server_handler.h"
#include "ms_task.h"
#include "ms_task_manager.h"
#include "ms_preloader.h"

static int has_prefix(const struct mg_str *uri, const struct mg_str *prefix) {
  return uri->len > prefix->len && memcmp(uri->p, prefix->p, prefix->len) == 0;
}

static int is_equal(const struct mg_str *s1, const struct mg_str *s2) {
  return s1->len == s2->len && memcmp(s1->p, s2->p, s2->len) == 0;
}


static void fill_pipe_json(char *json, int len, struct ms_ipipe *pipe) {
  snprintf(json + strlen(json), len - strlen(json), "{");
  snprintf(json + strlen(json), len - strlen(json), "\"id\": \"%p\",", pipe);
  snprintf(json + strlen(json), len - strlen(json), "\"pos\": %"INT64_FMT",", pipe->get_current_pos(pipe));
  snprintf(json + strlen(json), len - strlen(json), "\"len\": %"INT64_FMT, pipe->get_current_len(pipe));
  snprintf(json + strlen(json), len - strlen(json), "}");
}

static void fill_reader_json(char *json, int len, struct ms_ireader *reader) {
  snprintf(json + strlen(json), len - strlen(json), "{");
  snprintf(json + strlen(json), len - strlen(json), "\"id\": \"%p\",", reader);
  snprintf(json + strlen(json), len - strlen(json), "\"pos\": %"INT64_FMT",", reader->pos);
  snprintf(json + strlen(json), len - strlen(json), "\"len\": %"INT64_FMT, reader->len);
  snprintf(json + strlen(json), len - strlen(json), "}");
}

static void fill_task_json(char *json, int len, struct ms_task *task) {
  //  struct tm *timeinfo = localtime(&task->created_at);
  int64_t filesize = task->task.get_filesize(&task->task);
  int64_t completed = task->task.get_completed_size(&task->task);
  snprintf(json + strlen(json), len - strlen(json), "{");
  snprintf(json + strlen(json), len - strlen(json), "\"id\": \"%p\",", task);
  snprintf(json + strlen(json), len - strlen(json), "\"url\": \"%s\",", task->url.p);
  snprintf(json + strlen(json), len - strlen(json), "\"totalLength\": %"INT64_FMT",", filesize);
  snprintf(json + strlen(json), len - strlen(json), "\"completedLength\": %"INT64_FMT",", completed);
  snprintf(json + strlen(json), len - strlen(json), "\"speed\": 0,");
  snprintf(json + strlen(json), len - strlen(json), "\"bitmap\": \"%s\",", task->task.get_bitmap(&task->task));
  
  snprintf(json + strlen(json), len - strlen(json), "\"readers\":[");
  QUEUE *qr;
  struct ms_ireader *reader = NULL;
  QUEUE_FOREACH(qr, &task->readers) {
    if (reader != NULL) {
      snprintf(json + strlen(json), len - strlen(json), ",");
    }
    reader = QUEUE_DATA(qr, struct ms_ireader, node);
    fill_reader_json(json + strlen(json), len - (int)strlen(json), reader);
  }
  snprintf(json + strlen(json), len - strlen(json), "],");
  
  
  snprintf(json + strlen(json), len - strlen(json), "\"pipes\":[");
  QUEUE *qp;
  struct ms_ipipe *pipe = NULL;
  QUEUE_FOREACH(qp, &task->pipes) {
    if (pipe != NULL) {
      snprintf(json + strlen(json), len - strlen(json), ",");
    }
    pipe = QUEUE_DATA(qp, struct ms_ipipe, node);
    fill_pipe_json(json + strlen(json), len - (int)strlen(json), pipe);
  }
  snprintf(json + strlen(json), len - strlen(json), "]");
  
  snprintf(json + strlen(json), len - strlen(json), "}");
}

static void get_task_list_handler(struct mg_connection *nc, int ev, void *p) {
  if (ev != MG_EV_HTTP_REQUEST) {
    return;
  }
  
  struct http_message *hm = (struct http_message *)p;
  MS_ASSERT(mg_strcmp(hm->method, mg_mk_str("GET")) == 0);
  char url[MG_MAX_HTTP_REQUEST_SIZE] = {0};
  int url_len = 0;
  url_len = mg_get_http_var(&hm->query_string, "url", url, MG_MAX_HTTP_REQUEST_SIZE);
  struct mg_str url_str = MG_MK_STR(url);
  
  int capacity_len = 1024*1024;
  char *response = MS_MALLOC(capacity_len);
  memset(response, 0, capacity_len);
  int resp_len = (int)strlen(response);
  
  snprintf(response + resp_len, capacity_len - resp_len, "[");
  QUEUE *q;
  struct ms_task *task = NULL;
  int task_num = 0;
  QUEUE_FOREACH(q, &ms_default_server()->tasks) {
    task = QUEUE_DATA(q, struct ms_task, node);
    
    if (url_len <= 0 || is_equal(&url_str, &task->url)) {
      if (task_num > 0) {
        resp_len = (int)strlen(response);
        snprintf(response + resp_len, capacity_len - resp_len, ",");
      }
      resp_len = (int)strlen(response);
      fill_task_json(response + resp_len, capacity_len - resp_len, task);
      // TODO: more tasks MS_REALLOC
      
      task_num += 1;
    }
    
  }
  
  resp_len = (int)strlen(response);
  snprintf(response + resp_len, capacity_len - resp_len, "]");
  
  resp_len = (int)strlen(response);
  
  mg_send_head(nc, 200, resp_len, "Content-Type: application/json\r\nConnection: keep-alive");
  mg_send(nc, response, resp_len);
  
  MS_FREE(response);
}

static void get_task_handler(struct mg_connection *nc, int ev, void *p) {
  if (ev != MG_EV_HTTP_REQUEST) {
    return;
  }
  
  struct http_message *hm = (struct http_message *)p;
  MS_ASSERT(mg_strcmp(hm->method, mg_mk_str("GET")) == 0);
  
  QUEUE *q;
  struct ms_task *task = NULL;
  QUEUE_FOREACH(q, &ms_default_server()->tasks) {
    task = QUEUE_DATA(q, struct ms_task, node);
    char uri[32] = {0};
    snprintf(uri, 32, "/api/tasks/%p", task);
    if (mg_vcmp(&hm->uri, uri) == 0) {
      char json[1024*1024] = {0};
      fill_task_json(json, 1024*1024, task);
      int resp_len = (int)strlen(json);
      mg_send_head(nc, 200, resp_len, "Content-Type: application/json\r\nConnection: keep-alive");
      mg_send(nc, json, resp_len);
      return;
    }
  }
  
  mg_http_send_error(nc, 404, "not found");
}

static void preload_handler(struct mg_connection *nc, int ev, void *p) {
  struct ms_server *server = nc->user_data;
  if (ev == MG_EV_HTTP_REQUEST) {
    struct http_message *hm = (struct http_message *)p;
    if (mg_strcmp(hm->method, mg_mk_str("GET")) == 0) {
      
    } else if (mg_strcmp(hm->method, mg_mk_str("POST")) == 0) {
      char url[MG_MAX_HTTP_REQUEST_SIZE] = {0};
      mg_get_http_var(&hm->body, "url", url, MG_MAX_HTTP_REQUEST_SIZE);
      MS_DBG("%s", url);
      
      struct ms_task *task = ms_find_or_create_task(url, server);
      struct ms_preloader *preloader = ms_preloader_open(&task->task, 30*1024*1024);
      QUEUE_INSERT_TAIL(&server->preloaders, &preloader->node);
      task->task.add_reader(&task->task, (struct ms_ireader *)preloader);
    } else if (mg_strcmp(hm->method, mg_mk_str("DELETE")) == 0) {
      char url[MG_MAX_HTTP_REQUEST_SIZE] = {0};
      mg_get_http_var(&hm->body, "url", url, MG_MAX_HTTP_REQUEST_SIZE);
      MS_DBG("%s", url);
      
      QUEUE *q;
      struct ms_preloader *preloader = NULL;
      QUEUE_FOREACH(q, &ms_default_server()->preloaders) {
        preloader = QUEUE_DATA(q, struct ms_preloader, node);
        struct ms_task *task = (struct ms_task *)preloader->task;
        if (mg_vcmp(&task->url, url) == 0) {
          preloader->task->remove_reader(preloader->task, &preloader->reader);
          QUEUE_REMOVE(&preloader->node);
          ms_preloader_close(preloader);
        }
      }
    }
    
  }
}

void ms_api_handler(struct mg_connection *nc, int ev, void *p) {
  struct ms_server *server = nc->user_data;
  if (ev == MG_EV_HTTP_REQUEST) {
    static const struct mg_str api_tasks = MG_MK_STR("/api/tasks");
    static const struct mg_str api_preload = MG_MK_STR("/api/preload");
    struct http_message *hm = (struct http_message *)p;
    if (is_equal(&hm->uri, &api_tasks)) {
      return get_task_list_handler(nc, ev, p);
    } else if (has_prefix(&hm->uri, &api_tasks)) {
      return get_task_handler(nc, ev, p);
    } else if (has_prefix(&hm->uri, &api_preload)) {
      return preload_handler(nc, ev, p);
    } else {
      mg_serve_http(nc, hm, server->opts);
    }
  }
}
