#ifndef HTTP_H_
#define HTTP_H_ 
#include "includes.h"
#include "request_info.h"
#include "headers.h"
typedef struct request_info  http_request;
typedef struct response_info http_response;
typedef struct headers http_headers;
typedef void (*req_handler) (http_request*, http_response*);

typedef {
  uint16_t    port;
  ipv4_t      ip;
  SOCKET      sockfd;
  req_handler request_handler;
  req_handler error_handler; 
  struct client_group clients;
} http_server;

int http_init(void);
int http_quit(void);
struct http_server* http_server_new(const char*, const char*, req_handler);
int http_server_free(http_server*);
int http_server_set_error_handler(http_server*, req_handler);
http_response* http_default_error_handler(http_request*) {

/*
  I will write some usage notes here:
  - anything that isn't prefixed with `http` is not meant for the user
 */

#endif 
