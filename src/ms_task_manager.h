//
//  ms_task_manager.h
//  ms
//
//  Created by Jianguo Wu on 2019/5/24.
//  Copyright © 2019 libms. All rights reserved.
//

#ifndef ms_task_manager_h
#define ms_task_manager_h

#include "ms_server.h"
#include "ms_task.h"

void ms_remove_task_if_need(struct ms_server *server, double ts);

struct ms_task *ms_find_or_create_task(const char *url, struct ms_server *server);

#endif /* ms_task_manager_h */
