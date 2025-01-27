#include "request_parser.h"
#include "client_info.h"
#include "request_info.h"

int parse_request(struct client_info* client, http_constraints* constraints) {
  struct request_info* req = &client->request;
  size_t len = 0;
  char* q = client->buffer;
  char* begin = q;
  char* end = q + client->bufflen;
  *end = 0;
  
  while (q < end && strstr(q, "\r\n")) {
    if (req->state == STATE_GOT_NOTHING) {
      begin = q;
      q = strchr(q, ' ');
      if (!q)
        return HTTP_FAILURE;
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
        return HTTP_FAILURE; 

      ++q;
      begin = q;
      if (q >= end)
        return HTTP_FAILURE;
      if (*begin != '/')
        return HTTP_FAILURE;
      q = strchr(q, ' ');
      if (!q)
        return HTTP_FAILURE;
      *q = 0;
      if ((len = strlen(begin)) > constraints->request_max_uri_len)
        return HTTP_FAILURE;
      if (strstr(begin, ".."))
        return HTTP_FAILURE;
      memcpy(req->resource, begin, len);
      req->resource[len] = 0;
      req->resource_len  = len;
      
      ++q;
      begin = q;
      if (q >= end)
        return HTTP_FAILURE;
      q = strstr(q, "\r\n");
      if (!q)
        return HTTP_FAILURE;
      *q = 0;
      if (strcmp(begin, "HTTP/1.0") == 0) {
        int method = req->method;
        if (method != METHOD_GET && method != METHOD_HEAD && method != METHOD_POST)
          return HTTP_FAILURE; 
        req->version = HTTP_VERSION_1;
      }
      else if (strcmp(begin, "HTTP/1.1") == 0)
        req->version = HTTP_VERSION_1_1;
      else
        return HTTP_FAILURE;
      q += 2;

      req->state = STATE_GOT_LINE;
    }

    else if (req->state == STATE_GOT_LINE) {
      if (q < end && memcmp(q, "\r\n", 2) == 0) {
        req->state = STATE_GOT_HEADERS;
        return HTTP_SUCCESS;
      }
      if (req->headers->size > constraints->request_max_headers)
        return HTTP_FAILURE;

      size_t len = 0;
      begin = q;
      q = strchr(q, ':');
      if (!q)
        return HTTP_FAILURE;
      *q++ = 0;
      len += strlen(begin);
      if (q >= end)
        return HTTP_FAILURE;
      
      while (isspace(*q) && q < end) ++q;
      if (q >= end)
        return HTTP_FAILURE;
      char* p = strstr(q, "\r\n");
      if (!p)
        return HTTP_FAILURE;
      *p = 0;
      if (strchr(begin, '\n') || strchr(begin, '\r'))
        return HTTP_FAILURE;
      len += strlen(begin);
      if (len > constraints->request_max_header_len)
        return HTTP_FAILURE;
      add_header_request_info(req, begin, q);
      q = p + 2;
    }
    
    else if (req->state == STATE_GOT_HEADERS) {
      int method = req->method;
      if (method != METHOD_POST || method != METHOD_PUT || method != METHOD_PATCH) {
        req->state = STATE_GOT_ALL;
        return HTTP_SUCCESS; 
      }

      if (req->body_termination == BODYTERMI_LENGTH) {
        size_t length = req->length;
        if (req->length >= constraints->request_max_body_len)
          return HTTP_FAILURE;
        while (q < end && req->body_len < length) 
          req->body[req->body_len++] = *q++;
        if (req->body_len == length)
          req->state = STATE_GOT_ALL;
      }
      
      if (req->body_termination == BODYTERMI_CHUNKED) {
        if (req->chunk == 0) {
          char hex[8] = { 0 };
          char c = *q;
          int i  = 0;
          while (i < 7 && q < end && c >= '0' && c <= '9' || c >= 'a' && c <= 'f' || c >= 'A' && c <= 'F') {
            hex[i++] = c;
            c = *++q;
          }
          req->chunk = strtoul(hex, NULL, 16);
          if (q >= end || memcmp(q, "\r\n", 2) != 0)
            return HTTP_FAILURE;
          q += 2;
          if (req->chunk == 0) {
            req->state = STATE_GOT_ALL;
            return HTTP_SUCCESS;
          }
        }
        size_t chunk = req->chunk;
        while (q < end && chunk-- > 0)
          req->body[req->body_len++] = *q++;
        if (chunk == 0) {
          if (q >= end || memcmp(q, "\r\n", 2) != 0)
            return HTTP_FAILURE;
          q += 2;
        }
        req->chunk = chunk;
      }
    }
  }
  
  client->bufflen = MAX(0, client->bufflen - (q - client->buffer));
  if (client->bufflen > 0)
    memcpy(client->buffer, q, client->bufflen);
  return HTTP_SUCCESS;
}
