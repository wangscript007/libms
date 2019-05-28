//
//  ms_server_handler.h
//  ms
//
//  Created by Jianguo Wu on 2019/5/24.
//  Copyright © 2019 libms. All rights reserved.
//

#ifndef ms_server_handler_h
#define ms_server_handler_h

#include "ms_server.h"

void ms_api_handler(struct mg_connection *nc, int ev, void *p);


#endif /* ms_server_handler_h */
