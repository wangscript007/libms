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


static enum ms_server_status s_server_status = ms_server_status_idle;

static int s_exit_flag = 0;
static struct ms_server s_server;

struct ms_server *ms_default_server(void) {
  return &s_server;
}


static void init_media_server(struct ms_server *server) {
  QUEUE_INIT(&server->sessions);
  QUEUE_INIT(&server->tasks);
  QUEUE_INIT(&server->preloaders);
  QUEUE_INIT(&server->resources);
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
    MS_DBG("uri: %.*s", (int)hm->uri.len, hm->uri.p);

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

short ms_server_port(void) {
  return s_server.port;
}

void ms_start(short http_port, const char *path, void (*callback)(void)) {
  init_media_server(&s_server);
  strcpy(s_server.path, path);
  
  struct mg_connection *nc = (struct mg_connection *)0;
  
  MS_DBG("Starting web server on port %d", http_port);
  short current_port = http_port;
  for (; current_port < http_port + 100; ++current_port) {
    char port[16] = {0};
    snprintf(port, 16, "%d", current_port);
    nc = mg_bind(&s_server.mgr, port, server_handler);
    if (nc) {
      MS_DBG("success bind port: %s", port);
      break;
    } else {
      MS_DBG("failed bind port: %s", port);
    }
  }
  if (!nc) {
    return;
  }
  
  s_server.port = current_port;
  nc->user_data = &s_server;
  // Set up HTTP server parameters
  mg_set_protocol_http_websocket(nc);
  s_server.nc = nc;

  if (callback) {
    callback();
  }
  s_server_status = ms_server_status_running;
  while (s_exit_flag == 0) {
    mg_mgr_poll(&s_server.mgr, 1000);
    close_timeout_connections();
  }
  ms_remove_task_if_need(&s_server, 0);
  mg_mgr_free(&s_server.mgr);
  uninit_media_server(&s_server);
  s_server_status = ms_server_status_idle;
}

void ms_stop() {
  s_exit_flag = 1;
}

struct ms_start_param {
  short port;
  char path[MG_MAX_PATH];
};

static void *server_thread(void *argv) {
  struct ms_start_param *param = (struct ms_start_param *)argv;
  pthread_setname_np("libms");
  ms_start(param->port, param->path, NULL);
  MS_FREE(param);
  return NULL;
}

enum ms_server_status ms_server_current_status() {
  return s_server_status;
}


void ms_asnyc_start(short http_port, const char *path) {
  if (s_server_status != ms_server_status_idle) {
    return;
  }
  s_server_status = ms_server_status_starting;
  
  pthread_t thread_id = (pthread_t) 0;
  pthread_attr_t attr;
  
  (void) pthread_attr_init(&attr);
  (void) pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
  struct ms_start_param *param = (struct ms_start_param *)MS_MALLOC(sizeof(struct ms_start_param));
  memset(param, 0, sizeof(struct ms_start_param));
  param->port = http_port;
  strncpy(param->path, path, MG_MAX_PATH);
  pthread_create(&thread_id, &attr, server_thread, param);
  pthread_attr_destroy(&attr);
}

int ms_generate_url(const struct ms_url_param *input, char *out_url, size_t out_len) {
  MS_DBG("%s", input->url);
  struct mg_str encode = mg_url_encode_opt(mg_mk_str(input->url), mg_mk_str("._-$,;~()"), 0);
  int ret = snprintf(out_url, out_len - 1, "http://127.0.0.1:%d/stream/%s?url=%.*s", ms_default_server()->port, input->path, (int)encode.len, encode.p);
  MS_FREE((void *) encode.p);
  MS_DBG("%d, %s", ret, out_url);
  return ret;
}



static int s_base_url_id = 0;

static void get_play_url(struct ms_resource *resource, char *play_url, size_t play_url_len) {
  struct mg_str path = {0};
  mg_parse_uri(resource->origin_url, NULL, NULL, NULL, NULL, &path, NULL, NULL);
  const char *p = path.p, *end = p + path.len;
  const char *start = p;
  while (p < end) {
    if (*p == '/') {
      start = p + 1;
    }
    p++;
  }

  snprintf(play_url, play_url_len - 1, "http://127.0.0.1:%d/stream/%d/%.*s", ms_default_server()->port, resource->id, (int)(end - start), start);
}

int ms_generate_resource(const struct ms_url_param *input,
                         char *play_url,
                         size_t play_url_len,
                         int *resoueceId) {
  if (s_server_status != ms_server_status_running) {
    return -1;
  }
  QUEUE *q;
  struct ms_resource *resource = NULL;
  QUEUE_FOREACH(q, &ms_default_server()->resources) {
    resource = QUEUE_DATA(q, struct ms_resource, node);
    if (mg_strcmp(resource->origin_url, mg_mk_str(input->url)) == 0) {
      resource->ref += 1;
      get_play_url(resource, play_url, play_url_len);
      *resoueceId = resource->id;
      return 0;
    }
  }

  resource = (struct ms_resource *)MS_MALLOC(sizeof(struct ms_resource));
  memset(resource, 0, sizeof(struct ms_resource));
  s_base_url_id += 1;
  resource->id = s_base_url_id;
  resource->origin_url = mg_strdup_nul(mg_mk_str(input->url));
//  resource->name = mg_strdup_nul(mg_mk_str(input->name));
  resource->ref = 1;
  QUEUE_INIT(&resource->node);
  QUEUE_INSERT_TAIL(&ms_default_server()->resources, &resource->node);
  get_play_url(resource, play_url, play_url_len);

  *resoueceId = resource->id;
  return 0;
}

int ms_remove_resource(const int resoueceId) {
  if (s_server_status != ms_server_status_running) {
    return -1;
  }
  QUEUE *q;
  struct ms_resource *resource = NULL;
  QUEUE_FOREACH(q, &ms_default_server()->resources) {
    resource = QUEUE_DATA(q, struct ms_resource, node);
    if (resource->id == resoueceId) {
      resource->ref -= 1;
      if (resource->ref == 0) {
        QUEUE_REMOVE(q);
        MS_FREE((void *)resource->origin_url.p);
        MS_FREE(resource);
        return 0;
      }
      break;
    }
  }
  return 0;
}

struct ms_resource *ms_find_resource(const struct mg_str *uri) {
  if (s_server_status != ms_server_status_running) {
    return NULL;
  }
  
  QUEUE *q;
  struct ms_resource *resource = NULL;
  QUEUE_FOREACH(q, &ms_default_server()->resources) {
    resource = QUEUE_DATA(q, struct ms_resource, node);
    char url[64] = {0};
    snprintf(url, 64 - 1, "/stream/%d/", resource->id);
    struct mg_str str = mg_mk_str(url);
    if (has_prefix(uri, &str)) {
      return resource;
    }
  }
  return NULL;
}

struct ms_resource *ms_find_resource_by_id(const int resource_id) {
  if (s_server_status != ms_server_status_running) {
    return NULL;
  }
  QUEUE *q;
  struct ms_resource *resource = NULL;
  QUEUE_FOREACH(q, &ms_default_server()->resources) {
    resource = QUEUE_DATA(q, struct ms_resource, node);
    if (resource->id == resource_id) {
      return resource;
    }
  }
  return NULL;
}


