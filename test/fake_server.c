//
//  fake_server.c
//  libms
//
//  Created by Jianguo Wu on 2019/1/4.
//  Copyright © 2019 wujianguo. All rights reserved.
//

#include "fake_server.h"
#include "ms.h"

static struct ms_fake_server s_server = {0};

static void ev_handler(struct mg_connection *nc, int ev, void *p) {
  if (ev == MG_EV_HTTP_REQUEST) {
    struct http_message *hm = (struct http_message *)p;
    
    struct ms_fake_server *server = &s_server;
    QUEUE *q;
    struct ms_fake_nc *nct;
    QUEUE_FOREACH(q, &server->nc) {
      nct = QUEUE_DATA(q, struct ms_fake_nc, node);
      if (mg_strcmp(hm->uri, mg_mk_str(nct->uri)) == 0) {
        if (nct->type == ms_fake_type_normal) {
          mg_http_serve_file(nc, hm, nct->path, mg_mk_str("video/mp4"), mg_mk_str(""));
        } else if (nct->type == ms_fake_type_error) {
          mg_http_send_error(nc, 404, "Not Found");
        } else if (nct->type == ms_fake_type_close) {
          nc->flags |= MG_F_CLOSE_IMMEDIATELY;
        } else if (nct->type == ms_fake_type_redirect_1) {
          char url[MG_MAX_HTTP_REQUEST_SIZE] = {0};
          struct ms_fake_nc *nctf = nc_of(ms_fake_type_normal);
          fake_url(nctf, url, MG_MAX_PATH);
          mg_http_send_redirect(nc, 302, mg_mk_str(url), mg_mk_str(NULL));
        } else if (nct->type == ms_fake_type_redirect_2) {
          char url[MG_MAX_HTTP_REQUEST_SIZE] = {0};
          struct ms_fake_nc *nctf = nc_of(ms_fake_type_redirect_1);
          fake_url(nctf, url, MG_MAX_PATH);
          mg_http_send_redirect(nc, 302, mg_mk_str(url), mg_mk_str(NULL));
        } else if (nct->type == ms_fake_type_redirect_3) {
          char url[MG_MAX_HTTP_REQUEST_SIZE] = {0};
          struct ms_fake_nc *nctf = nc_of(ms_fake_type_redirect_2);
          fake_url(nctf, url, MG_MAX_PATH);
          mg_http_send_redirect(nc, 302, mg_mk_str(url), mg_mk_str(NULL));
        } else if (nct->type == ms_fake_type_redirect_4) {
          char url[MG_MAX_HTTP_REQUEST_SIZE] = {0};
          struct ms_fake_nc *nctf = nc_of(ms_fake_type_redirect_3);
          fake_url(nctf, url, MG_MAX_PATH);
          mg_http_send_redirect(nc, 302, mg_mk_str(url), mg_mk_str(NULL));
        }
        return;
      }
    }
    mg_http_send_error(nc, 404, "Not Found");
  }
}

static void add_nc(struct ms_fake_server *server, char *path, char *uri, enum ms_fake_type type) {
  struct ms_fake_nc *nc = (struct ms_fake_nc *)MS_MALLOC(sizeof(struct ms_fake_nc));
  memset(nc, 0, sizeof(struct ms_fake_nc));
  QUEUE_INIT(&nc->node);
  memcpy(nc->path, path, MG_MAX_PATH - 1);
  memcpy(nc->uri, uri, MG_MAX_PATH - 1);
  nc->type = type;
  cs_stat_t st;
  if (mg_stat(path, &st) == 0) {
    nc->filesize = st.st_size;
  }
  MS_ASSERT(nc->filesize > 0);
  QUEUE_INSERT_TAIL(&server->nc, &nc->node);
  mg_register_http_endpoint(ms_default_server()->nc, nc->uri, ev_handler);

}

void start_fake_server(void) {
  QUEUE_INIT(&s_server.nc);
  char path[MG_MAX_PATH] = {0};
  strcpy(path, ms_default_server()->path);
  strcat(path, "/build/wildo.mp4");
  add_nc(&s_server, path, "/fake/normal.mp4",     ms_fake_type_normal);
  add_nc(&s_server, path, "/fake/error.mp4",      ms_fake_type_error);
  add_nc(&s_server, path, "/fake/close.mp4",      ms_fake_type_close);
  add_nc(&s_server, path, "/fake/redirect1.mp4",  ms_fake_type_redirect_1);
  add_nc(&s_server, path, "/fake/redirect2.mp4",  ms_fake_type_redirect_2);
  add_nc(&s_server, path, "/fake/redirect3.mp4",  ms_fake_type_redirect_3);
  add_nc(&s_server, path, "/fake/redirect4.mp4",  ms_fake_type_redirect_4);
}

void fake_url(struct ms_fake_nc *nc, char *url, int url_len) {
    snprintf(url, url_len, "http://127.0.0.1:%s%s", ms_default_server()->port, nc->uri);
}

struct ms_fake_nc *nc_of(enum ms_fake_type type) {
  QUEUE *q;
  struct ms_fake_nc *nct;
  QUEUE_FOREACH(q, &s_server.nc) {
    nct = QUEUE_DATA(q, struct ms_fake_nc, node);
    if (nct->type == type) {
      return nct;
    }
  }
  abort();
  return NULL;
}
