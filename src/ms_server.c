//
//  ms_server.c
//  libms
//
//  Created by Jianguo Wu on 2018/11/21.
//  Copyright © 2018 wujianguo. All rights reserved.
//

#include "ms.h"
#include "ms_server.h"
#include "ms_session.h"
#include "ms_mem_storage.h"
#include "ms_http_pipe.h"
#include "ms_task.h"
#include <pthread.h>
#include <time.h>

#define MS_STRINGIFY(v) MS_STRINGIFY_HELPER(v)
#define MS_STRINGIFY_HELPER(v) #v

#define MS_VERSION_STRING_BASE  MS_STRINGIFY(MS_VERSION_MAJOR) "." \
MS_STRINGIFY(MS_VERSION_MINOR) "." \
MS_STRINGIFY(MS_VERSION_PATCH)

#if MS_VERSION_IS_RELEASE
# define MS_VERSION_STRING  MS_VERSION_STRING_BASE
#else
# define MS_VERSION_STRING  MS_VERSION_STRING_BASE "-" MS_VERSION_SUFFIX
#endif

char * ms_current_time_str() {
  static char buffer[26] = {0};
  time_t rawtime;
  time (&rawtime);
  struct tm* tm_info = localtime(&rawtime);
  strftime(buffer, 26, "%H:%M:%S", tm_info);
  return buffer;
}

unsigned int ms_version(void) {
  return MS_VERSION_HEX;
}


const char* ms_version_string(void) {
  return MS_VERSION_STRING;
}

static const char *method_strings[] = {
#define XX(num, name, string) #string,
  MS_HTTP_METHOD_MAP(XX)
#undef XX
};

#ifndef ELEM_AT
# define ELEM_AT(a, i, v) ((unsigned int) (i) < ARRAY_SIZE(a) ? (a)[(i)] : (v))
#endif

const char * ms_http_method_str (enum ms_http_method m) {
  return ELEM_AT(method_strings, m, "<unknown>");
}

enum ms_http_method ms_http_method_enum(struct mg_str str) {
#define XX(num, name, string) if (mg_strcmp(str, mg_mk_str(#string)) == 0) { return num;}
  MS_HTTP_METHOD_MAP(XX)
#undef XX
  return MS_HTTP_UNKNOWN;
}


char *ms_str_of_ev(int ev) {
  if (ev == MG_EV_POLL) {
    return "MG_EV_POLL";
  } else if (ev == MG_EV_ACCEPT) {
    return "MG_EV_ACCEPT";
  } else if (ev == MG_EV_CONNECT) {
    return "MG_EV_CONNECT";
  } else if (ev == MG_EV_RECV) {
    return "MG_EV_RECV";
  } else if (ev == MG_EV_SEND) {
    return "MG_EV_SEND";
  } else if (ev == MG_EV_CLOSE) {
    return "MG_EV_CLOSE";
  } else if (ev == MG_EV_TIMER) {
    return "MG_EV_TIMER";
  } else if (ev == MG_EV_HTTP_REQUEST) {
    return "MG_EV_HTTP_REQUEST";
  } else if (ev == MG_EV_HTTP_REPLY) {
    return "MG_EV_HTTP_REPLY";
  } else if (ev == MG_EV_HTTP_CHUNK) {
    return "MG_EV_HTTP_CHUNK";
  } else if (ev == MG_EV_WEBSOCKET_CONTROL_FRAME) {
    return "MG_EV_WEBSOCKET_CONTROL_FRAME";
  } else {
    return "unknown";
  }
}


static int s_exit_flag = 0;
static struct ms_server s_server;

struct ms_server *ms_default_server(void) {
  return &s_server;
}


static void init_media_server(struct ms_server *server) {
  QUEUE_INIT(&server->sessions);
  QUEUE_INIT(&server->tasks);
  mg_mgr_init(&server->mgr, NULL);
  server->opts.document_root = ".";
  server->opts.enable_directory_listing = "yes";

#ifdef __APPLE__
  signal(SIGPIPE, SIG_IGN);
#endif
}

static void uninit_media_server(struct ms_server *server) {
  // TODO:
}

static int is_websocket(const struct mg_connection *nc) {
  return nc->flags & MG_F_IS_WEBSOCKET;
}


static struct ms_ipipe *open_pipe(const struct mg_str url,
                           int64_t pos,
                           int64_t len,
                           struct ms_ipipe_callback callback) {
  return (struct ms_ipipe *)ms_http_pipe_create(url, pos, len, callback);
}

static struct ms_istorage *open_storage(void) {
  return (struct ms_istorage *)ms_mem_storage_open();
}

static struct ms_session *find_session(struct mg_connection *nc, struct ms_server *server) {
  QUEUE *q;
  struct ms_session *session = NULL;
  QUEUE_FOREACH(q, &server->sessions) {
    session = QUEUE_DATA(q, struct ms_session, node);
    if (session->connection == nc) {
      return session;
    }
  }
  return NULL;
}

static void remove_task_if_need(struct ms_server *server, double ts) {
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

static struct ms_task *find_or_create_task(const char *url, struct ms_server *server) {
  QUEUE *q;
  struct ms_task *task = NULL;
  QUEUE_FOREACH(q, &server->tasks) {
    task = QUEUE_DATA(q, struct ms_task, node);
    if (mg_strcmp(task->url, mg_mk_str(url)) == 0) {
      return task;
    }
  }
  remove_task_if_need(server, 5);
  struct ms_factory factory = {
    open_storage,
    open_pipe
  };
  task = ms_task_open(mg_mk_str(url), factory);
  QUEUE_INSERT_TAIL(&server->tasks, &task->node);
  return task;
}

static void dispatch_task() {
  
}

static void close_timeout_connections() {
  
}

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

struct ms_monitor_client {
  QUEUE   node;
  struct mg_connection *nc;
};

static void api_handler(struct mg_connection *nc, int ev, void *p) {
  struct ms_server *server = nc->user_data;
  if (ev == MG_EV_HTTP_REQUEST) {
    static const struct mg_str api_tasks = MG_MK_STR("/api/tasks");
    struct http_message *hm = (struct http_message *)p;
    if (is_equal(&api_tasks, &hm->uri)) {
      return get_task_list_handler(nc, ev, p);
    } else if (has_prefix(&hm->uri, &api_tasks)) {
      return get_task_handler(nc, ev, p);
    } else {
      mg_serve_http(nc, hm, server->opts);
    }
  }
}

static void stream_handler(struct mg_connection *nc, int ev, void *p) {
  if (ev != MG_EV_POLL && ev != MG_EV_SEND) {
    MS_DBG("handler:%p, %s", nc, ms_str_of_ev(ev));
  }
  struct ms_server *server = nc->user_data;
  if (ev == MG_EV_HTTP_REQUEST) {
    struct http_message *hm = (struct http_message *)p;
    MS_ASSERT(mg_strcmp(hm->method, mg_mk_str("GET")) == 0 || mg_strcmp(hm->method, mg_mk_str("HEAD")) == 0);
    char url[MG_MAX_HTTP_REQUEST_SIZE] = {0};
    mg_get_http_var(&hm->query_string, "url", url, MG_MAX_HTTP_REQUEST_SIZE);
    MS_DBG("%s", url);
    
    int i = 0;
    for (i = 0; i < MG_MAX_HTTP_HEADERS && hm->header_names[i].len > 0; i++) {
      struct mg_str hn = hm->header_names[i];
      struct mg_str hv = hm->header_values[i];
//      MS_DBG("%.*s: %.*s", (int)hn.len, hn.p, (int)hv.len, hv.p);
      printf("%.*s: %.*s\n", (int)hn.len, hn.p, (int)hv.len, hv.p);
    }
    
    struct ms_session *session = find_session(nc, server);
    if (session) {
      session->task->remove_reader(session->task, (struct ms_ireader *)session);
      QUEUE_REMOVE(&session->node);
      ms_session_close(session);
    }

    struct ms_task *task = find_or_create_task(url, server);
    if (task) {
      struct ms_session *session = ms_session_open(nc, hm, &task->task);
      QUEUE_INSERT_TAIL(&server->sessions, &session->node);
      task->task.add_reader(&task->task, (struct ms_ireader *)session);
//      ms_task_add_reader(task, (struct ms_ireader *)session);
      ms_session_try_transfer_data(session);
      ms_session_close_if_need(session);
    }

  }
}

static void server_handler(struct mg_connection *nc, int ev, void *p) {
  if (ev != MG_EV_POLL && ev != MG_EV_SEND && ev != MG_EV_RECV && ev != MG_EV_WEBSOCKET_CONTROL_FRAME) {
    MS_DBG("handler:%p, %s", nc, ms_str_of_ev(ev));
  }
  
  struct ms_server *server = nc->user_data;
  
  if (ev == MG_EV_HTTP_REQUEST) {
    static const struct mg_str stream_prefix = MG_MK_STR("/stream/");
    static const struct mg_str api_prefix = MG_MK_STR("/api/");
    struct http_message *hm = (struct http_message *)p;
    if (has_prefix(&hm->uri, &stream_prefix)) {
      return stream_handler(nc, ev, p);
    } else if (has_prefix(&hm->uri, &api_prefix)) {
      return api_handler(nc, ev, p);
    }
    mg_serve_http(nc, hm, server->opts);
  } else if (ev == MG_EV_SEND) {
    struct ms_session *session = find_session(nc, server);
    int *num_sent_bytes = (int *)p;
    if (session) {
      //            MS_ASSERT(*num_sent_bytes > 0);
      if (*num_sent_bytes < 0) {
        // TODO: handle error.
        MS_DBG("%d", errno);
        nc->flags |= MG_F_CLOSE_IMMEDIATELY;
      } else {
        session->reader.on_send((struct ms_ireader *)session, *num_sent_bytes);
      }
    }
  } else if (ev == MG_EV_WEBSOCKET_HANDSHAKE_REQUEST) {
    MS_DBG("websocket handshake request");
  } else if (ev == MG_EV_WEBSOCKET_HANDSHAKE_DONE) {
    MS_DBG("websocket handshake done");
  } else if (ev == MG_EV_WEBSOCKET_FRAME) {
    struct websocket_message *wm = (struct websocket_message *)p;
    struct mg_str d = {(char *) wm->data, wm->size};
    MS_DBG("websocket frame %.*s", (int)d.len, d.p);
    mg_send_websocket_frame(nc, WEBSOCKET_OP_TEXT, "", 0);
  } else if (ev == MG_EV_CLOSE) {
    if (is_websocket(nc)) {
      MS_DBG("websocket close.");
    }
    struct ms_session *session = find_session(nc, server);
    if (session) {
//      ms_task_remove_reader(session->task, (struct ms_ireader *)session);
      session->task->remove_reader(session->task, (struct ms_ireader *)session);
      QUEUE_REMOVE(&session->node);
      ms_session_close(session);
    }
  } else if (ev == MG_EV_TIMER) {
    MS_DBG("%p timer", nc);
  }
}

void ms_start(const char *http_port, const char *path, void (*callback)(void)) {
  init_media_server(&s_server);
  strcpy(s_server.path, path);
  
  struct mg_connection *nc;
  
  MS_DBG("Starting web server on port %s", http_port);
  nc = mg_bind(&s_server.mgr, http_port, server_handler);
  if (nc == NULL) {
    MS_DBG("Failed to create listener");
    return;
  }
  strcpy(s_server.port, http_port);
  nc->user_data = &s_server;
  // Set up HTTP server parameters
  mg_set_protocol_http_websocket(nc);
  s_server.nc = nc;
  
  mg_register_http_endpoint(nc, "/stream/?*", stream_handler);
  mg_register_http_endpoint(nc, "/api/tasks", get_task_list_handler);
  mg_register_http_endpoint(nc, "/api/tasks/*", get_task_handler);

  if (callback) {
    callback();
  }
  
  while (s_exit_flag == 0) {
    mg_mgr_poll(&s_server.mgr, 1000);
    close_timeout_connections();
  }
  remove_task_if_need(&s_server, 0);
  mg_mgr_free(&s_server.mgr);
  uninit_media_server(&s_server);
  
}

void ms_stop() {
  s_exit_flag = 1;
}

static void *server_thread(void *argv) {
  pthread_setname_np("libms");
  ms_start("8090", "", NULL);
  return NULL;
}

void ms_asnyc_start() {
  pthread_t thread_id = (pthread_t) 0;
  pthread_attr_t attr;
  
  (void) pthread_attr_init(&attr);
  (void) pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
  
  pthread_create(&thread_id, &attr, server_thread, NULL);
  pthread_attr_destroy(&attr);
}

int ms_generate_url(const struct ms_url_param *input, char *out_url, size_t out_len) {
  MS_DBG("%s", input->url);
  struct mg_str encode = mg_url_encode_opt(mg_mk_str(input->url), mg_mk_str("._-$,;~()"), 0);
  int ret = snprintf(out_url, out_len - 1, "http://127.0.0.1:%s/stream/%s?url=%.*s", ms_default_server()->port, input->path, (int)encode.len, encode.p);
  MS_FREE((void *) encode.p);
  MS_DBG("%d, %s", ret, out_url);
  return ret;
}

