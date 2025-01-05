#include "request_parser.h "
#include "includes.h"
#include "client_info.h"
#include "request_info.h"

int parse_request(struct client_info* client) {
  struct request_info* req = &client->request;
  size_t  len = 0;
  char* q = client->buffer;
  char* begin = q;
  char* end = q + client->bufflen;
  *end = 0;
  
  while (q < end && strstr(q, "\r\n")) {
    if (req->state == STATE_GOT_NOTHING) {
      begin = q;
      q = strchr(q, ' ');
      if (!q)
        return 1;
      *q = 0;
      if (strcmp(begin, "GET") == 0)
        req->method = METHOD_GET;
      else if (strcmp(begin, "HEAD") == 0)
        req->method = METHOD_HEAD;
      else if (strcmp(begin, "POST") == 0)
        req->method = METHOD_POST;
      else if (strcmp(begin, "PUT") == 0)
        req->method = METHOD_PUT;
      else if (strcmp(begin, "DELETE") == 0)
        req->method = METHOD_DELETE;
      else if (strcmp(begin, "CONNECT") == 0)
        req->method = METHOD_CONNECT;
      else if (strcmp(begin, "OPTIONS") == 0)
        req->method = METHOD_OPTIONS;
      else if (strcmp(begin, "TRACE") == 0)
        req->method = METHOD_TRACE;
      else if (strcmp(begin, "PATCH") == 0)
        req->method = METHOD_PATCH;
      else
        return 1; 

      ++q;
      begin = q;
      if (q >= end)
        return 1;
      if (*begin != '/')
        return 1;
      q = strchr(q, ' ');
      if (!q)
        return 1;
      *q = 0;
      if ((len = strlen(begin)) > RESOURCE_BUFFLEN)
        return 1;
      if (strstr(begin, ".."))
        return 1;
      memcpy(req->resource, begin, len);
      req->resource[len] = 0;
      req->resource_len  = len;
      
      ++q;
      begin = q;
      if (q >= end)
        return 1;
      q = strstr(q, "\r\n");
      if (!q)
        return 1;
      *q = 0;
      if (strcmp(begin, "HTTP/1.0") == 0)
        req->version = HTTP_VERSION_1;
      else if (strcmp(begin, "HTTP/1.1") == 0)
        req->version = HTTP_VERSION_1_1;
      else
        return 1;
      q += 2;

      req->state = STATE_GOT_LINE;
    }

    else if (req->state == STATE_GOT_LINE) {
      if (q < end && memcmp(q, "\r\n", 2) == 0) {
        req->state = STATE_GOT_HEADERS;
        return 0;
      }

      begin = q;
      q = strchr(q, ':');
      if (!q)
        return 1;
      *q++ = 0;
      if (q >= end)
        return 1;

      while (isspace(*q) && q < end) ++q;
      if (q >= end)
        return 1;
      char* p = strstr(q, "\r\n");
      if (!p)
        return 1;
      *p = 0;
      if (strchr(begin, '\n') || strchr(begin, '\r'))
        return 1; 
      add_header_request_info(req, begin, q);
      q = p + 2;
    }
    
    else if (req->state == STATE_GOT_HEADERS) {
      while (q < end && req->body_len < BODY_BUFFLEN) 
        req->body[req->body_len++] = *q++; 

      if (q < end && req->body_len == BODY_BUFFLEN)
        return 1;
    }

  }
  client->bufflen = MAX(0, client->bufflen - (q - client->buffer));
  if (client->bufflen > 0)
    memcpy(client->buffer, q, client->bufflen);
  return 0;
}
