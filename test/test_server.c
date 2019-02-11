//
//  test_server.c
//  libms
//
//  Created by Jianguo Wu on 2018/11/28.
//  Copyright © 2018 wujianguo. All rights reserved.
//

#include "run_tests.h"
#include "ms.h"
#include "fake_server.h"

struct ms_test_client {
  int64_t pos;
  int64_t size;
  int     fp;
  int     complete;
  on_case_done callback;
};

static void check_buf(struct ms_test_client *client, const char *buf, int64_t pos, size_t len) {
  char *fbuf = MS_MALLOC(len);
  size_t read = pread(client->fp, fbuf, len, pos);
  MS_ASSERT(read == len);
  int i = 0;
  for (; i<len; ++i) {
    MS_ASSERT(fbuf[i] == buf[i]);
  }
  MS_FREE(fbuf);
  // MS_DBG("%x,%x,%x,%x,%x %lld, %zu, %lld", buf[0], buf[1], buf[2], buf[3], buf[4], pos, len, pos + len);
}


static void test_handler(struct mg_connection *nc, int ev, void *ev_data) {
  if (ev != MG_EV_POLL && ev != MG_EV_RECV && ev != MG_EV_HTTP_CHUNK) {
    MS_DBG("%s", ms_str_of_ev(ev));
  }
  
  struct ms_test_client *client = (struct ms_test_client *)nc->user_data;
  
  if (ev == MG_EV_RECV) {
    
  } else if (ev == MG_EV_HTTP_CHUNK) {
    struct http_message *hm = (struct http_message *)ev_data;
    if (hm->body.len == 0) {
      return;
    }
    struct mg_str *v = mg_get_http_header(hm, "Content-Length");
    int64_t size = to64(v->p);
    MS_ASSERT(size==client->size);
    //        MS_DBG("%lld, %zu, %lld", client->pos, hm->body.len, client->pos + hm->body.len);
    check_buf(client, hm->body.p, client->pos, hm->body.len);
    client->pos += hm->body.len;
    nc->flags |= MG_F_DELETE_CHUNK;
  } else if (ev == MG_EV_HTTP_REPLY) {
    nc->flags |= MG_F_CLOSE_IMMEDIATELY;
    client->complete = 1;
  } else if (ev == MG_EV_CLOSE) {
    MS_ASSERT(client->complete);
    if (client->callback) {
      client->callback();
    }
    close(client->fp);
    MS_FREE(client);
  }
}

static void run_test(int64_t pos, int64_t len, enum ms_fake_type type, on_case_done callback) {
  char origin_url[MG_MAX_HTTP_REQUEST_SIZE] = {0};
  char ms_url[MG_MAX_HTTP_REQUEST_SIZE] = {0};
  
  struct ms_fake_nc *nct = nc_of(ms_fake_type_normal);
  fake_url(nct, origin_url, MG_MAX_PATH);
  struct ms_url_param param = {origin_url, "test.mp4"};
  ms_generate_url(&param, ms_url, MG_MAX_HTTP_REQUEST_SIZE);
  MS_DBG("%s, %s", origin_url, ms_url);
  
  MS_ASSERT(pos + len <= nct->filesize);
  char extra_headers[128] = {0};
  if (len == 0) {
    snprintf(extra_headers, 128, "Range: bytes=%" INT64_FMT "-\r\n", pos);
  } else {
    snprintf(extra_headers, 128, "Range: bytes=%" INT64_FMT "-%" INT64_FMT "\r\n", pos, pos + len - 1);
  }
  struct mg_connection *nc = mg_connect_http(&ms_default_server()->mgr, test_handler, ms_url, extra_headers, NULL);
  struct ms_test_client *client = MS_MALLOC(sizeof(struct ms_test_client));
  memset(client, 0, sizeof(struct ms_test_client));
  client->callback = callback;
  client->fp = open(nct->path, O_RDONLY);
  client->pos = pos;
  if (len == 0) {
    client->size = nct->filesize - pos;
  } else {
    client->size = len;
  }
  nc->user_data = client;
}

//static void timer_handler(struct mg_connection *nc, int ev, void *ev_data)
//{
//    if (ev == MG_EV_TIMER)
//    {
//        run_test(2332258, 0, NULL);
//    }
//}

void test_server_1(on_case_done callback) {
  run_test(0, 0, ms_fake_type_normal, callback);
}

void test_server_2(on_case_done callback) {
  run_test(120, 38475, ms_fake_type_normal, callback);
}

void test_server_3(on_case_done callback) {
  run_test(3934, 0, ms_fake_type_normal, callback);
}

void test_server_4(on_case_done callback) {
  run_test(120, 38475, ms_fake_type_normal, callback);
}

void test_server_5(on_case_done callback) {
  run_test(23444, 38474, ms_fake_type_normal, callback);
}

void test_server_6(on_case_done callback) {
  run_test(3934, 0, ms_fake_type_normal, callback);
}



struct ms_test_server_case {
  on_case_done callback;
  enum ms_fake_type type;
  int expect_code;
  int check;
};

static void test_server_error_handler(struct mg_connection *nc, int ev, void *ev_data) {
  struct ms_test_server_case *test_case = (struct ms_test_server_case *)nc->user_data;

  if (ev == MG_EV_HTTP_REPLY) {
    MS_ASSERT(test_case->check == 0);
    struct http_message *hm = (struct http_message *)ev_data;
    MS_ASSERT(hm->resp_code == test_case->expect_code);
    test_case->check = 1;
    nc->flags |= MG_F_CLOSE_IMMEDIATELY;
  } else if (ev == MG_EV_CLOSE) {
    MS_ASSERT(test_case->check == 1);
    test_case->callback();
    MS_FREE(test_case);
  }
}

static struct ms_test_server_case *run_server_test(enum ms_fake_type type, on_case_done callback) {
  struct ms_test_server_case *test_case = (struct ms_test_server_case *)MS_MALLOC(sizeof(struct ms_test_server_case));
  memset(test_case, 0, sizeof(struct ms_test_server_case));
  test_case->callback = callback;
  test_case->type = type;

  char origin_url[MG_MAX_HTTP_REQUEST_SIZE] = {0};
  char ms_url[MG_MAX_HTTP_REQUEST_SIZE] = {0};
  
  struct ms_fake_nc *nct = nc_of(type);
  fake_url(nct, origin_url, MG_MAX_PATH);
  struct ms_url_param param = {origin_url, "test.mp4"};
  ms_generate_url(&param, ms_url, MG_MAX_HTTP_REQUEST_SIZE);
  MS_DBG("%s, %s", origin_url, ms_url);
  
  char extra_headers[128] = {0};
  int64_t pos = 0;
  snprintf(extra_headers, 128, "Range: bytes=%" INT64_FMT "-\r\n", pos);
  struct mg_connection *nc = mg_connect_http(&ms_default_server()->mgr, test_server_error_handler, ms_url, extra_headers, NULL);
  nc->user_data = test_case;
  return test_case;
}

void test_server_error(on_case_done callback) {
  struct ms_test_server_case *test_case = run_server_test(ms_fake_type_error, callback);
  test_case->expect_code = 404;
}

void test_server_close(on_case_done callback) {
  struct ms_test_server_case *test_case = run_server_test(ms_fake_type_close, callback);
  test_case->expect_code = 502;
}

void test_server_redirect1(on_case_done callback) {
  run_test(0, 0, ms_fake_type_redirect_1, callback);
}

void test_server_redirect2(on_case_done callback) {
  run_test(0, 0, ms_fake_type_redirect_2, callback);
}

void test_server_redirect3(on_case_done callback) {
  run_test(0, 0, ms_fake_type_redirect_3, callback);
}

void test_server_redirect4(on_case_done callback) {
  struct ms_test_server_case *test_case = run_server_test(ms_fake_type_error, callback);
  test_case->expect_code = 310;
}


struct ms_test_invalid_server_case {
  on_case_done callback;
};

static void test_invalid_handler(struct mg_connection *nc, int ev, void *ev_data) {
  if (ev != MG_EV_POLL) {
    MS_DBG("%s", ms_str_of_ev(ev));
  }

  if (ev == MG_EV_CLOSE) {
    struct ms_test_invalid_server_case *test_case = (struct ms_test_invalid_server_case *)nc->user_data;
    test_case->callback();
  }
}

void test_server_invalid(on_case_done callback) {
  struct ms_test_invalid_server_case *test_case = (struct ms_test_invalid_server_case *)MS_MALLOC(sizeof(struct ms_test_invalid_server_case));
  memset(test_case, 0, sizeof(struct ms_test_invalid_server_case));
  test_case->callback = callback;
  
  char origin_url[MG_MAX_HTTP_REQUEST_SIZE] = "http://127.0.0.1/test.mp4";
  char ms_url[MG_MAX_HTTP_REQUEST_SIZE] = {0};

  
  struct ms_url_param param = {origin_url, "test.mp4"};
  ms_generate_url(&param, ms_url, MG_MAX_HTTP_REQUEST_SIZE);
  MS_DBG("%s, %s", origin_url, ms_url);
  
  char extra_headers[128] = {0};
  int64_t pos = 0;
  snprintf(extra_headers, 128, "Range: bytes=%" INT64_FMT "-\r\n", pos);
  struct mg_connection *nc = mg_connect_http(&ms_default_server()->mgr, test_invalid_handler, ms_url, extra_headers, NULL);
  nc->user_data = test_case;
}
