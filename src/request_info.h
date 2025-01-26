#ifndef REQUEST_INFO_H_
#define REQUEST_INFO_H_

#include "includes.h"
#define RESOURCE_BUFFLEN 255

enum {
  STATE_GOT_NOTHING,
  STATE_GOT_LINE,
  STATE_GOT_HEADERS, 
  STATE_GOT_ALL,
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

enum {
  BODYTERMI_LENGTH,
  BODYTERMI_CHUNKED,
  BODYTERMI_NONE
};

struct request_info {
  char   method;
  char*  resource;
  size_t resource_len;
  char   version;
  char   body[REQUEST_BODY_BUFFLEN + 1];
  size_t body_len;
  struct headers* headers;

  // non HTTP-related fields
  char state;
  SOCKET client_socket;
  struct sockaddr_in client_address;
  int body_termination;
  int length;
  int chunk; 
};

int make_request_info(struct request_info*, SOCKET, struct sockaddr_in*);
int free_request_info(struct request_info*);
int reset_request_info(struct request_info*, SOCKET, struct sockaddr_in*);
int add_header_request_info(struct request_info*, const char*, const char*);
#endif
