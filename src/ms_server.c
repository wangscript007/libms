//
//  ms_server.c
//  libms
//
//  Created by Jianguo Wu on 2018/11/21.
//  Copyright © 2018 wujianguo. All rights reserved.
//

#include "ms.h"
#include "ms_server.h"
#include "ms_server_handler.h"
#include "ms_stream_handler.h"
#include "ms_task_manager.h"
#include "ms_session_manager.h"
#include "ms_preloader.h"
#include "ms_mem_storage.h"
#include "ms_http_pipe.h"
#include <pthread.h>


static int s_exit_flag = 0;
static struct ms_server s_server;

struct ms_server *ms_default_server(void) {
  return &s_server;
}


static void init_media_server(struct ms_server *server) {
  QUEUE_INIT(&server->sessions);
  QUEUE_INIT(&server->tasks);
  QUEUE_INIT(&server->preloaders);
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

static void close_timeout_connections() {
  
}


static int has_prefix(const struct mg_str *uri, const struct mg_str *prefix) {
  return uri->len > prefix->len && memcmp(uri->p, prefix->p, prefix->len) == 0;
}

static void server_handler(struct mg_connection *nc, int ev, void *p) {
  if (ev != MG_EV_POLL && ev != MG_EV_SEND && ev != MG_EV_RECV) {
    MS_DBG("handler:%p, %s", nc, ms_str_of_ev(ev));
  }
  
  struct ms_server *server = nc->user_data;
  
  if (ev == MG_EV_HTTP_REQUEST) {
    static const struct mg_str stream_prefix = MG_MK_STR("/stream/");
    static const struct mg_str api_prefix = MG_MK_STR("/api/");
    struct http_message *hm = (struct http_message *)p;
    if (has_prefix(&hm->uri, &stream_prefix)) {
      return ms_stream_handler(nc, ev, p);
    } else if (has_prefix(&hm->uri, &api_prefix)) {
      return ms_api_handler(nc, ev, p);
    }
    mg_serve_http(nc, hm, server->opts);
  } else if (ev == MG_EV_SEND) {
    struct ms_session *session = ms_find_session(nc, server);
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
  } else if (ev == MG_EV_CLOSE) {
    struct ms_session *session = ms_find_session(nc, server);
    if (session) {
//      ms_task_remove_reader(session->task, (struct ms_ireader *)session);
      session->task->remove_reader(session->task, &session->reader);
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
  
//  mg_register_http_endpoint(nc, "/stream/?*", stream_handler);
//  mg_register_http_endpoint(nc, "/api/tasks", get_task_list_handler);
//  mg_register_http_endpoint(nc, "/api/tasks/*", get_task_handler);

  if (callback) {
    callback();
  }
  
  while (s_exit_flag == 0) {
    mg_mgr_poll(&s_server.mgr, 1000);
    close_timeout_connections();
  }
  ms_remove_task_if_need(&s_server, 0);
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

