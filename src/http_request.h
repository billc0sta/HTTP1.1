#ifndef HTTP_REQUEST_H_
#define HTTP_REQUEST_H_

#include "includes.h"

enum {
  METHOD_GET,
  METHOD_POST,
  METHOD_HEAD,
  METHOD_PUT,
  METHOD_DELETE,
  METHOD_CONNECT,
  METHOD_OPTIONS,
  METHOD_TRACE,
  METHOD_PATCH,
  METHOD_NONE
};

typedef struct {
  char   method;
  char   version;
  char*  uri;
  size_t uri_len;
  char*  body;
  size_t body_len; 
  http_headers* headers;

  // internal use 
  char state;
  SOCKET conn_socket;
  struct sockaddr_in conn_address;
  int body_termination;
  size_t length;
  size_t chunk; 
} http_request;

int http_request_make(http_request*, SOCKET, struct sockaddr_in*, http_constraints*);
int http_request_free(http_request*);
int http_request_reset(http_request*, SOCKET, struct sockaddr_in*);
int http_request_add_header(http_request*, const char*, const char*);

#endif
