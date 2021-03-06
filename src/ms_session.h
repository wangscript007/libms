//
//  ms_session.h
//  libms
//
//  Created by Jianguo Wu on 2018/11/21.
//  Copyright © 2018 wujianguo. All rights reserved.
//

#ifndef ms_session_h
#define ms_session_h


#include "ms_server.h"
//#include "ms_task.h"

enum ms_media_type {
  ms_media_type_unknown,
  ms_media_type_m3u8,
  ms_media_type_mp4,
  ms_media_type_flv
};

struct ms_media_meta {
  enum ms_media_type  type;
};

struct ms_session {
  struct ms_ireader       reader;
  struct mg_connection    *connection;
  enum ms_http_method     method;
  struct ms_itask         *task;
  struct mbuf             buf;
  struct ms_media_meta    *meta;
//  int                     waiting;
  
  QUEUE                   node;
  
  int                     fp;
};

struct ms_session *ms_session_open(struct mg_connection *nc, struct http_message *hm, struct ms_itask *task);

size_t ms_session_try_transfer_data(struct ms_session *session);

void ms_session_close_if_need(struct ms_session *session);

void ms_session_close(struct ms_session *session);


#endif /* ms_session_h */

