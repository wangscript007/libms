//
//  ms_session_manager.c
//  ms
//
//  Created by Jianguo Wu on 2019/5/24.
//  Copyright © 2019 libms. All rights reserved.
//

#include "ms_session_manager.h"

struct ms_session *ms_find_session(struct mg_connection *nc, struct ms_server *server) {
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
