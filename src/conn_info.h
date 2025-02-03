#ifndef CONN_INFO_H_
#define CONN_INFO_H_
#include "includes.h" 
#include "http_request.h"
#include "http_response.h"
#define CONN_BUFF_LEN 1024

struct conn_info {
  SOCKET               sockfd;
  struct sockaddr_in   addr;
  char                 buffer[CONN_BUFF_LEN + 1];
  size_t               buff_len;
  size_t               buff_used; 
  char                 used;
  http_request  request;
  http_response response; 
};

struct conn_group {
  size_t         len;
  size_t         cap;
  http_constraints* constraints; 
  struct conn_info* data;
};

struct conn_info* conn_group_add(struct conn_group*, SOCKET, struct sockaddr_in* s);
int conn_info_drop(struct conn_info*);
struct conn_group conn_group_make(http_constraints*);
struct conn_info* conn_info_new(http_constraints*);
int conn_info_free(struct conn_info*);
int conn_group_free(struct conn_group*);
int conn_group_wait(struct conn_group*, SOCKET, fd_set*);
int conn_info_reset(struct conn_info*, http_constraints*);

#endif
