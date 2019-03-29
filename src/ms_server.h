//
//  ms_server.h
//  libms
//
//  Created by Jianguo Wu on 2018/11/29.
//  Copyright © 2018 wujianguo. All rights reserved.
//

#ifndef ms_server_h
#define ms_server_h

#include "ms.h"

struct ms_istorage {
  int64_t (*get_filesize)(struct ms_istorage *st);
  void    (*set_filesize)(struct ms_istorage *st, int64_t filesize);
  int64_t (*get_estimate_size)(struct ms_istorage *st);
  void    (*set_content_size)(struct ms_istorage *st, int64_t from, int64_t size);
  int64_t (*get_completed_size)(struct ms_istorage *st);
  void    (*cached_from)(struct ms_istorage *st, int64_t from, int64_t *pos, int64_t *len);
  size_t  (*write)(struct ms_istorage *st, const char *buf, int64_t pos, size_t len);
  size_t  (*read)(struct ms_istorage *st, char *buf, int64_t pos, size_t len);
  void    (*close)(struct ms_istorage *st);
  char *  (*get_bitmap)(struct ms_istorage *st);
};

struct ms_ireader {
  int64_t pos;
  int64_t len;
  size_t  sending;
  size_t  header_sending;
  
  int64_t         req_pos;
  int64_t         req_len;
  
  
  void (*on_send)(struct ms_ireader *reader, int len);
  void (*on_recv)(struct ms_ireader *reader, int64_t pos, size_t len);
  void (*on_filesize)(struct ms_ireader *reader, int64_t filesize);
  void (*on_content_size_from)(struct ms_ireader *reader, int64_t pos, int64_t size);
  //    void (*on_close)(struct ms_reader *reader);
  void (*on_error)(struct ms_ireader *reader, int code);
  
  QUEUE   node;
};

struct ms_itask {
  void (*add_reader)(struct ms_itask *task, struct ms_ireader *reader);
  struct mg_str (*content_type)(struct ms_itask *task);
  size_t (*read)(struct ms_itask *task, char *buf, int64_t pos, size_t len);
  int64_t (*get_filesize)(struct ms_itask *task);
  int64_t (*get_estimate_size)(struct ms_itask *task);
  int64_t (*get_completed_size)(struct ms_itask *task);
  void (*remove_reader)(struct ms_itask *task, struct ms_ireader *reader);
  void (*close)(struct ms_itask *task);
  int (*get_errno)(struct ms_itask *task);
  char * (*get_bitmap)(struct ms_itask *task);
};

struct ms_ipipe;
struct ms_ipipe_callback {
  int64_t (*get_filesize)(struct ms_ipipe *pipe);
  void    (*on_header)(struct ms_ipipe *pipe, struct http_message *hm);
  void    (*on_filesize)(struct ms_ipipe *pipe, int64_t filesize);
  void    (*on_content_size)(struct ms_ipipe *pipe, int64_t pos, int64_t size);
  void    (*on_recv)(struct ms_ipipe *pipe, const char *buf, int64_t pos, size_t len);
  void    (*on_redirect)(struct ms_ipipe *pipe, struct mg_str location);
  void    (*on_complete)(struct ms_ipipe *pipe);
  void    (*on_close)(struct ms_ipipe *pipe, int code);
};

struct ms_ipipe {
  QUEUE   node;

  int64_t (*get_req_len)(struct ms_ipipe *pipe);
  int64_t (*get_current_pos)(struct ms_ipipe *pipe);
  int64_t (*get_current_len)(struct ms_ipipe *pipe);
  void    (*connect)(struct ms_ipipe *pipe);
  void    (*close)(struct ms_ipipe *pipe);
  
  void    *user_data;
  struct ms_ipipe_callback callback;
};

struct ms_idispatch {
  
};

struct ms_factory {
  struct ms_istorage*  (*open_storage)(void);
  struct ms_ipipe*     (*open_pipe)(const struct mg_str url,
                                   int64_t pos,
                                   int64_t len,
                                   struct ms_ipipe_callback callback);
  
};


/* Request Methods */
#define MS_HTTP_METHOD_MAP(XX)      \
  XX(0,  DELETE,      DELETE)       \
  XX(1,  GET,         GET)          \
  XX(2,  HEAD,        HEAD)         \
  XX(3,  POST,        POST)         \
  XX(4,  PUT,         PUT)          \
  /* pathological */                \
  XX(5,  CONNECT,     CONNECT)      \
  XX(6,  OPTIONS,     OPTIONS)      \
  XX(7,  TRACE,       TRACE)        \
  /* WebDAV */                      \
  XX(8,  COPY,        COPY)         \
  XX(9,  LOCK,        LOCK)         \
  XX(10, MKCOL,       MKCOL)        \
  XX(11, MOVE,        MOVE)         \
  XX(12, PROPFIND,    PROPFIND)     \
  XX(13, PROPPATCH,   PROPPATCH)    \
  XX(14, SEARCH,      SEARCH)       \
  XX(15, UNLOCK,      UNLOCK)       \
  XX(16, BIND,        BIND)         \
  XX(17, REBIND,      REBIND)       \
  XX(18, UNBIND,      UNBIND)       \
  XX(19, ACL,         ACL)          \
  /* subversion */                  \
  XX(20, REPORT,      REPORT)       \
  XX(21, MKACTIVITY,  MKACTIVITY)   \
  XX(22, CHECKOUT,    CHECKOUT)     \
  XX(23, MERGE,       MERGE)        \
  /* upnp */                        \
  XX(24, MSEARCH,     M-SEARCH)     \
  XX(25, NOTIFY,      NOTIFY)       \
  XX(26, SUBSCRIBE,   SUBSCRIBE)    \
  XX(27, UNSUBSCRIBE, UNSUBSCRIBE)  \
  /* RFC-5789 */                    \
  XX(28, PATCH,       PATCH)        \
  XX(29, PURGE,       PURGE)        \
  /* CalDAV */                      \
  XX(30, MKCALENDAR,  MKCALENDAR)   \
  /* RFC-2068, section 19.6.1.2 */  \
  XX(31, LINK,        LINK)         \
  XX(32, UNLINK,      UNLINK)       \
  /* icecast */                     \
  XX(33, SOURCE,      SOURCE)       \
  XX(100, UNKNOWN,    UNKNOWN)

enum ms_http_method
{
#define XX(num, name, string) MS_HTTP_##name = num,
  MS_HTTP_METHOD_MAP(XX)
#undef XX
};

const char * ms_http_method_str (enum ms_http_method m);

enum ms_http_method ms_http_method_enum(struct mg_str str);

#endif /* ms_server_h */

