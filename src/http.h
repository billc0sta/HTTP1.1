#include "includes.h"
#include "request_info.h"
#include "headers.h"
typedef struct request_info http_request;
typedef struct headers http_headers;

typedef {
  uint16_t port;
  ipv4_t   ip;
  SOCKET   sockfd;
  struct client_group clients;
} http_server;

void http_init(void);
void http_quit(void);
struct http_server* http_make_server(const char*);
void http_destroy_server(struct http_server*);

/*
  I will write some usage notes here:
  - almost everything that isn't prefixed with `http` is not meant for the user.
 */
