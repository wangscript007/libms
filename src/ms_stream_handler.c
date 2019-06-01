//
//  ms_stream_handler.c
//  ms
//
//  Created by Jianguo Wu on 2019/5/24.
//  Copyright © 2019 libms. All rights reserved.
//

#include "ms_stream_handler.h"
#include "ms_session_manager.h"
#include "ms_task_manager.h"

void ms_stream_handler(struct mg_connection *nc, int ev, void *p) {
  if (ev != MG_EV_POLL && ev != MG_EV_SEND) {
    MS_DBG("handler:%p, %s", nc, ms_str_of_ev(ev));
  }
  struct ms_server *server = nc->user_data;
  if (ev == MG_EV_HTTP_REQUEST) {
    struct http_message *hm = (struct http_message *)p;
    MS_ASSERT(mg_strcmp(hm->method, mg_mk_str("GET")) == 0 || mg_strcmp(hm->method, mg_mk_str("HEAD")) == 0);
    char url[MG_MAX_HTTP_REQUEST_SIZE] = {0};
    mg_get_http_var(&hm->query_string, "url", url, MG_MAX_HTTP_REQUEST_SIZE);
    if (strlen(url) == 0) {
      struct ms_resource *resource = ms_find_resource(&hm->uri);
      if (resource) {
        strncpy(url, resource->origin_url.p, MG_MAX_HTTP_REQUEST_SIZE);
      }
    }
    MS_DBG("%s", url);
    
    int i = 0;
    for (i = 0; i < MG_MAX_HTTP_HEADERS && hm->header_names[i].len > 0; i++) {
      struct mg_str hn = hm->header_names[i];
      struct mg_str hv = hm->header_values[i];
      //      MS_DBG("%.*s: %.*s", (int)hn.len, hn.p, (int)hv.len, hv.p);
      printf("%.*s: %.*s\n", (int)hn.len, hn.p, (int)hv.len, hv.p);
    }
    
    struct ms_session *session = ms_find_session(nc, server);
    if (session) {
      session->task->remove_reader(session->task, (struct ms_ireader *)session);
      QUEUE_REMOVE(&session->node);
      ms_session_close(session);
    }
    
    struct ms_task *task = ms_find_or_create_task(url, server);
    session = ms_session_open(nc, hm, &task->task);
    QUEUE_INSERT_TAIL(&server->sessions, &session->node);
    task->task.add_reader(&task->task, (struct ms_ireader *)session);
    ms_session_try_transfer_data(session);
    ms_session_close_if_need(session);
  }
}
