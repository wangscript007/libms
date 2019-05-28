//
//  ms_session_manager.h
//  ms
//
//  Created by Jianguo Wu on 2019/5/24.
//  Copyright © 2019 libms. All rights reserved.
//

#ifndef ms_session_manager_h
#define ms_session_manager_h

#include "ms_server.h"
#include "ms_session.h"

struct ms_session *ms_find_session(struct mg_connection *nc, struct ms_server *server);

#endif /* ms_session_manager_h */
