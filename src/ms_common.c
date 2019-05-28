//
//  ms_common.c
//  ms
//
//  Created by Jianguo Wu on 2019/5/24.
//  Copyright © 2019 libms. All rights reserved.
//

#include "ms_common.h"
#include "ms.h"
#include <time.h>

#define MS_STRINGIFY(v) MS_STRINGIFY_HELPER(v)
#define MS_STRINGIFY_HELPER(v) #v

#define MS_VERSION_STRING_BASE  MS_STRINGIFY(MS_VERSION_MAJOR) "." \
MS_STRINGIFY(MS_VERSION_MINOR) "." \
MS_STRINGIFY(MS_VERSION_PATCH)

#if MS_VERSION_IS_RELEASE
# define MS_VERSION_STRING  MS_VERSION_STRING_BASE
#else
# define MS_VERSION_STRING  MS_VERSION_STRING_BASE "-" MS_VERSION_SUFFIX
#endif

char * ms_current_time_str() {
  static char buffer[26] = {0};
  time_t rawtime;
  time (&rawtime);
  struct tm* tm_info = localtime(&rawtime);
  strftime(buffer, 26, "%H:%M:%S", tm_info);
  return buffer;
}

unsigned int ms_version(void) {
  return MS_VERSION_HEX;
}


const char* ms_version_string(void) {
  return MS_VERSION_STRING;
}

static const char *method_strings[] = {
#define XX(num, name, string) #string,
  MS_HTTP_METHOD_MAP(XX)
#undef XX
};

#ifndef ELEM_AT
# define ELEM_AT(a, i, v) ((unsigned int) (i) < ARRAY_SIZE(a) ? (a)[(i)] : (v))
#endif

const char * ms_http_method_str (enum ms_http_method m) {
  return ELEM_AT(method_strings, m, "<unknown>");
}

enum ms_http_method ms_http_method_enum(struct mg_str str) {
#define XX(num, name, string) if (mg_strcmp(str, mg_mk_str(#string)) == 0) { return num;}
  MS_HTTP_METHOD_MAP(XX)
#undef XX
  return MS_HTTP_UNKNOWN;
}


char *ms_str_of_ev(int ev) {
  if (ev == MG_EV_POLL) {
    return "MG_EV_POLL";
  } else if (ev == MG_EV_ACCEPT) {
    return "MG_EV_ACCEPT";
  } else if (ev == MG_EV_CONNECT) {
    return "MG_EV_CONNECT";
  } else if (ev == MG_EV_RECV) {
    return "MG_EV_RECV";
  } else if (ev == MG_EV_SEND) {
    return "MG_EV_SEND";
  } else if (ev == MG_EV_CLOSE) {
    return "MG_EV_CLOSE";
  } else if (ev == MG_EV_TIMER) {
    return "MG_EV_TIMER";
  } else if (ev == MG_EV_HTTP_REQUEST) {
    return "MG_EV_HTTP_REQUEST";
  } else if (ev == MG_EV_HTTP_REPLY) {
    return "MG_EV_HTTP_REPLY";
  } else if (ev == MG_EV_HTTP_CHUNK) {
    return "MG_EV_HTTP_CHUNK";
  } else if (ev == MG_EV_WEBSOCKET_CONTROL_FRAME) {
    return "MG_EV_WEBSOCKET_CONTROL_FRAME";
  } else {
    return "unknown";
  }
}
