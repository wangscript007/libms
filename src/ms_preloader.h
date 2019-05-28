//
//  ms_preloader.h
//  ms
//
//  Created by Jianguo Wu on 2019/5/24.
//  Copyright © 2019 libms. All rights reserved.
//

#ifndef ms_preloader_h
#define ms_preloader_h

#include "ms_server.h"

struct ms_preloader {
  struct ms_ireader       reader;
  struct ms_itask         *task;
  QUEUE                   node;
};

struct ms_preloader *ms_preloader_open(struct ms_itask *task, int64_t len);

void ms_preloader_close(struct ms_preloader *preloader);

#endif /* ms_preloader_h */
