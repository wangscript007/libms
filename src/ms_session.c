//
//  ms_session.c
//  libms
//
//  Created by Jianguo Wu on 2018/11/21.
//  Copyright © 2018 wujianguo. All rights reserved.
//

#include "ms_session.h"

#define MS_F_HEADER_SEND MG_F_USER_1
//#define MS_SEND_BUF_LIMIT   (2*1024*1024)
#define MS_SEND_BUF_LIMIT   MG_MAX_HTTP_SEND_MBUF


static void parse_media(struct ms_session *session, const char *buf, size_t len) {
  if (session->meta) {
    return;
  }
  char m3u8_header[16] = "#EXTM3U";
  if (memcmp(buf, m3u8_header, strlen(m3u8_header)) == 0) {
    session->meta = (struct ms_media_meta *)MS_MALLOC(sizeof(struct ms_media_meta));
    session->meta->type = ms_media_type_m3u8;
  }
}

static void check_buf(struct ms_session *session, const char *buf, int64_t pos, size_t len)
{
  // if (session->fp == 0) {
  //   return;
  // }
  // char *fbuf = MS_MALLOC(len);
  // size_t read = pread(session->fp, fbuf, len, pos);
  // MS_ASSERT(read == len);
  // int i = 0;
  // for (; i<len; ++i) {
  //   MS_ASSERT(fbuf[i] == buf[i]);
  // }
  // MS_FREE(fbuf);
  // MS_DBG("session: %p (%" INT64_FMT ", %zu, %" INT64_FMT ")", session, pos, len, pos + len);
}

static int http_parse_range_header(const struct mg_str *header, int64_t *a,
                                   int64_t *b) {
  /*
   * There is no snscanf. Headers are not guaranteed to be NUL-terminated,
   * so we have this. Ugh.
   */
  int result;
  char *p = (char *) MS_MALLOC(header->len + 1);
  memcpy(p, header->p, header->len);
  p[header->len] = '\0';
  result = sscanf(p, "bytes=%" INT64_FMT "-%" INT64_FMT, a, b);
  MS_FREE(p);
  return result;
}

static size_t try_transfer_data(struct ms_session *session) {
  MS_ASSERT(session->reader.sending + session->reader.header_sending == session->connection->send_mbuf.len);
  if (session->reader.sending > MS_SEND_BUF_LIMIT / 2) { // make sure read at least 1M.
    return 0;
  }
  
  if (session->reader.len == 0) {
    // TODO: support reader.len == 0
    return 0;
  }
  if (session->task->get_filesize(session->task) == 0) {
    return 0;
  }
  
  if (session->method == MS_HTTP_GET && !session->meta && session->reader.req_pos == 0) {
    char buf[1024] = {0};
    size_t wanted = 1024;
    if (wanted > session->task->get_filesize(session->task)) {
      wanted = session->task->get_filesize(session->task);
    }
    size_t read = session->task->read(session->task, buf, 0, wanted);
    parse_media(session, buf, read);
//    if (session->meta->type == ms_media_type_m3u8) {
//      read = session->task->read(session->task, buf, 0, wanted);
//      return 0;
//    }
  }
  
  if (!(session->connection->flags & MS_F_HEADER_SEND)) {
    session->connection->flags |= MS_F_HEADER_SEND;
    MS_DBG("session:%p, send head content-length: %" INT64_FMT", filesize: %"INT64_FMT, session, session->reader.len, session->task->get_filesize(session->task));
    // TODO: 1. keep-alive 2. proxy headers like `content-type`, 3. reader.len == 0
    
    struct mg_str ct = session->task->content_type(session->task);
    char header_str[MG_MAX_HTTP_SEND_MBUF] = {0};
    snprintf(header_str, MG_MAX_HTTP_SEND_MBUF, "Content-Type: %s\r\n"
             "Accept-Ranges: bytes\r\n"
             "Content-Range: bytes %" INT64_FMT "-%" INT64_FMT "/%" INT64_FMT "\r\n"
             "Connection: keep-alive", ct.p, session->reader.pos, session->reader.len + session->reader.pos - 1, session->task->get_filesize(session->task));
    MS_DBG("%s", header_str);
//    MS_ASSERT(session->reader.len - session->reader.pos - 1 > 0);
    int resp_code = 206;
    if (session->method == MS_HTTP_HEAD) {
      resp_code = 200;
    }
    mg_send_head(session->connection, resp_code, session->reader.len , header_str);
    session->reader.header_sending += session->connection->send_mbuf.len;
  }

  if (session->method == MS_HTTP_HEAD) {
    session->connection->flags |= MG_F_SEND_AND_CLOSE;
    return 0;
  }
  
  int64_t read_from = session->reader.pos + session->buf.len + session->connection->send_mbuf.len - session->reader.header_sending;
  size_t wanted = session->buf.size - session->buf.len;
  if (session->reader.len - session->reader.sending < wanted) {
    wanted = session->reader.len - session->reader.sending;
  }
  int64_t size = session->task->get_filesize(session->task);
//  session->task
  if (size == 0) {
    size = session->task->get_estimate_size(session->task);
  }

  if (size > 0 && read_from + (int64_t)wanted > size) {
    wanted = size - read_from;
  }
  
  MS_ASSERT(session->reader.req_len == 0 || read_from + (int64_t)wanted <= session->reader.req_pos + session->reader.req_len);
  size_t read = session->task->read(session->task, session->buf.buf + session->buf.len, read_from, wanted);
  check_buf(session, session->buf.buf + session->buf.len, read_from, read);
  session->buf.len += read;
  
  if (session->buf.len == 0) {
    return 0;
  }
  //    check_buf(session, buf, session->reader.pos, read);
  
  size_t len = session->buf.len;
  if (len + session->reader.header_sending > session->buf.size) { // make sure send_mbuf <= 2M
    len -= session->reader.header_sending;
  }
  mg_send(session->connection, session->buf.buf, (int)len);
  check_buf(session, session->buf.buf, session->reader.pos + session->reader.sending, len);
  MS_ASSERT(session->reader.req_len == 0 || session->reader.pos + (int64_t)session->reader.sending + (int64_t)len <= session->reader.req_pos + session->reader.req_len);
  
//  MS_DBG("session:%p %x,%x,%x,%x,%x, send %lld, %zu, read:%zu, send_mbuf:%zu",session, session->buf.buf[0], session->buf.buf[1], session->buf.buf[2], session->buf.buf[3], session->buf.buf[4], read_from, len, read, session->connection->send_mbuf.len);
  
  session->reader.sending += len;
  mbuf_remove(&session->buf, len);
  
  //    if (session->reader.sending == session->reader.len) {
  //        MS_DBG("MG_F_SEND_AND_CLOSE");
  //        session->connection->flags |= MG_F_SEND_AND_CLOSE;
  //    }
  return len;
  
}

static void on_send(struct ms_ireader *reader, int num_sent_bytes) {
  struct ms_session *session = (struct ms_session *)reader;
  if (session->connection->flags & MG_F_SEND_AND_CLOSE) {
    return;
  }
  int body_len = num_sent_bytes;
  if (session->reader.header_sending > 0) {
    if ((int)session->reader.header_sending > num_sent_bytes) {
      MS_DBG("session:%p header_sending:%zu, len:%d", session, session->reader.header_sending, body_len);
      session->reader.header_sending -= num_sent_bytes;
      body_len = 0;
    } else {
      body_len = num_sent_bytes - (int)session->reader.header_sending;
      MS_DBG("session:%p header_sending:%zu, len:%d", session, session->reader.header_sending, body_len);
      session->reader.header_sending = 0;
    }
  }
  
//  MS_DBG("session:%p pos:%lld, len:%d, left:%lld, num_sent_bytes:%d, send_mbuf.len:%zu", session, session->reader.pos, body_len, session->reader.len, num_sent_bytes, session->connection->send_mbuf.len);
//  MS_ASSERT(session->reader.len > 0);
  if (session->reader.len == 0) {
    return;
  }
  
  session->reader.pos += body_len;
  if (session->reader.len > 0) {
    MS_ASSERT(session->reader.len >= body_len);
    session->reader.len -= body_len;
  }
  session->reader.sending -= body_len;
  
  if (session->reader.len == 0 && session->reader.sending == 0) {
//    session->connection->flags |= MG_F_CLOSE_IMMEDIATELY;
//    session->waiting = 1;
    
    session->task->remove_reader(session->task, (struct ms_ireader *)session);
    QUEUE_REMOVE(&session->node);
    ms_session_close(session);
    return;
  }
  
  try_transfer_data(session);
  ms_session_close_if_need(session);
}

static void on_filesize(struct ms_ireader *reader, int64_t filesize) {
//  if (reader->len > 0) {
//    return;
//  }
//  struct ms_session *session = (struct ms_session *)reader;
//  if (filesize > 0) {
//    reader->len = filesize - reader->pos;
//  }
}

static void on_content_size_from(struct ms_ireader *reader, int64_t pos, int64_t size) {
  if (reader->len > 0) {
    return;
  }
  struct ms_session *session = (struct ms_session *)reader;
  int64_t filesize = session->task->get_filesize(session->task);
  if (filesize > 0) {
    reader->len = filesize - reader->pos;
  }
}


static void on_recv(struct ms_ireader *reader, int64_t pos, size_t len) {
  struct ms_session *session = (struct ms_session *)reader;
  //    MS_DBG("session:%p recv:(%lld, %zu, %lld)", session, pos, len, pos + len);
  try_transfer_data(session);
  ms_session_close_if_need(session);
}

static void on_error(struct ms_ireader *reader, int code) {
  struct ms_session *session = (struct ms_session *)reader;
  ms_session_close_if_need(session);
}

//static void on_close(struct ms_reader *reader)
//{
//    // TODO:
//
//}

struct ms_session *ms_session_open(struct mg_connection *nc, struct http_message *hm, struct ms_itask *task) {
  struct ms_session *session = MS_MALLOC(sizeof(struct ms_session));
  memset(session, 0, sizeof(struct ms_session));
  MS_DBG("session:%p, nc:%p", session, nc);
  QUEUE_INIT(&session->node);
  QUEUE_INIT(&session->reader.node);
  session->connection = nc;
  mbuf_init(&session->buf, MS_SEND_BUF_LIMIT);
  session->connection->flags &= ~MS_F_HEADER_SEND;
  //    session->buf
//  session->waiting = 0;
  session->method = ms_http_method_enum(hm->method);
  session->task = task;
  int n = 0;
  int64_t r1 = 0, r2 = 0;
  struct mg_str *range_hdr = mg_get_http_header(hm, "Range");
  if (range_hdr != NULL &&
      (n = http_parse_range_header(range_hdr, &r1, &r2)) > 0 && r1 >= 0 &&
      r2 >= 0) {
    /* If range is specified like "400-", set second limit to 0 */
    if (n == 1) {
      r2 = 0;
    } else {
      r2 = r2 - r1 + 1;
    }
    MS_ASSERT(r1 >= 0 && r2 >= 0);
    MS_DBG("session:%p, Range (%" INT64_FMT ", %" INT64_FMT ")  %.*s", session, r1, r2, (int)range_hdr->len, range_hdr->p);
  }
  session->reader.pos = r1;
  session->reader.len = r2;
  session->reader.req_pos = r1;
  session->reader.req_len = r2;
  //    session->reader.len = 364491696;
  session->reader.on_send  = on_send;
  session->reader.on_recv  = on_recv;
  //    session->reader.on_close = on_close;
  session->reader.on_error = on_error;
  session->reader.on_filesize = on_filesize;
  session->reader.on_content_size_from = on_content_size_from;
  
  int64_t filesize = task->get_filesize(task);
  if (session->reader.len == 0 && filesize > 0) {
    session->reader.len = filesize - session->reader.pos;
  }
  // session->fp = open("/Users/wujianguo/Documents/build/wildo.mp4", O_RDONLY);
  return session;
}


size_t ms_session_try_transfer_data(struct ms_session *session) {
  return try_transfer_data(session);
}

void ms_session_close_if_need(struct ms_session *session) {
  int task_code = session->task->get_errno(session->task);
  if (task_code == 0 || session->reader.sending > 0 || session->reader.header_sending > 0) {
    return;
  }

  if (session->connection->flags & MS_F_HEADER_SEND) {
    session->connection->flags |= MG_F_SEND_AND_CLOSE;
  } else if (!(session->connection->flags & MG_F_SEND_AND_CLOSE)) {
    mg_http_send_error(session->connection, task_code, NULL);
    session->connection->flags |= MG_F_SEND_AND_CLOSE;
  }
}

void ms_session_close(struct ms_session *session) {
  MS_DBG("session:%p, sending: %zu, len:%"INT64_FMT, session, session->reader.sending, session->reader.len);
  if (session->fp > 0) {
    close(session->fp);
  }
  mbuf_free(&session->buf);
  MS_FREE(session);
}

