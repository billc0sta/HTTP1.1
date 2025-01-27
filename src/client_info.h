#ifndef CLIENT_INFO_H_
#define CLIENT_INFO_H_
#include "includes.h" 
#include "request_info.h"
#include "response_info.h"
#define CLIENT_BUFFER_LEN 1024

struct client_info {
  SOCKET               sockfd;
  struct sockaddr_in   addr;
  char                 buffer[CLIENT_BUFFER_LEN + 1];
  size_t               buff_len;
  size_t               buff_used; 
  char                 used;
  http_request  request;
  http_response response; 
};

struct client_group {
  size_t         len;
  size_t         cap;
  http_constraints* constraints; 
  struct client_info* data;
};

struct client_info* add_client(struct client_group*, SOCKET, struct sockaddr_in* s);
int drop_client(struct client_info*);
struct client_group make_client_group(http_constraints*);
int free_clients_group(struct client_group*);
int ready_clients(struct client_group*, SOCKET, fd_set*);
int reset_client_info(struct client_info*);
int print_client_address(struct client_info*);

#endif
