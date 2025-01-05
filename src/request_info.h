#ifndef REQUEST_INFO_H_
#define REQUEST_INFO_H_

#include "includes.h"
#define RESOURCE_BUFFLEN 255
#define BODY_BUFFLEN (1024 * 1024 * 2)

enum {
  STATE_GOT_NOTHING,
  STATE_GOT_LINE,
  STATE_GOT_HEADERS, 
  STATE_GOT_BODY,
}; 

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

struct request_info {
  char   method;
  char*  resource;
  size_t resource_len;
  char   version;
  char   body[BODY_BUFFLEN + 1];
  size_t body_len;
  headers* headers;

  // non HTTP-related fields
  char state;
  SOCKET client_socket;
  struct sockaddr_in client_address; 
};

int make_request_info(struct request_info*);
int free_request_info(struct request_info*);
int reset_request_info(struct request_info*);
int add_header_request_info(struct request_info* req, char* header, char* value);
#endif
