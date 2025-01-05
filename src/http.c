#include "http.h" 
#include "includes.h"
#include "client_info.h"
#include "request_parser.h"

void http_init(void) {
#ifdef _WIN32
  WSADATA data;
  int res;
  if (res = WSAStartup(MAKEWORD(2, 2), &data)) {
    fprintf(stderr, "[main] WSAStartup() failed - %d.\n", res);
    exit(1);
  }
#endif
}

void http_quit(void) {  
#ifdef _WIN32
  WSACleanup();
#endif
}

struct http_server* http_make_server(const char* ip, const char* port) {
  if (!ip || !port) {
    http_log(stderr, "[http_make_server] passed NULL pointers for mandatory parameters");
    return NULL;
  }

  ip = (strcmp(ip, "0.0.0.0") == 0)
    ip = 0;
  
  struct addrinfo hints;
  struct addrinfo* binder;
  memset(&hints, 0, sizeof(hints));
  hints.ai_family   = AF_INET;
  hints.ai_socktype = SOCK_STREAM;
  hints.ai_flags    = AI_PASSIVE;

  getaddrinfo(ip, port, &hints, &binder);
  SOCKET sockfd = socket(binder->ai_family, binder->ai_socktype, binder->ai_protocol);
  if (server_sockfd == INVALID_SOCKET) {
    http_log(stderr, "[http_make_server] socket() failed - %d.\n", GET_ERROR());
    return NULL;
  }
  if (bind(server_sockfd, binder->ai_addr, (int)binder->ai_addrlen)) {
    http_log(stderr, "[http_make_server] bind() failed - %d.\n", GET_ERROR());
    return NULL; 
  }
  if (listen(server_sockfd, 10)) {
    http_log(stderr, "[http_make_server] listen() failed - %d.\n", GET_ERROR());
    return NULL;
  }

  http_server* server = malloc(sizeof(http_server));
  if (!server) {
    http_log(stderr, "[http_make_server] failed to allocate memory.\n");
    return NULL
  }
  
  server->ip      = ntohl(((struct sockaddr_in*)&binder->ai_addr)->SIN_ADDR);
  server->port    = ntohs(((struct sockaddr_in*)&binder->ai_addr)->sin_port);
  server->sockfd  = sockfd;   
  server->clients = make_client_group();
  freeaddrinfo(binder);
  return server;
}

void http_destroy_server(struct http_server* server) {
  if (!server) {
    http_log(stderr, "[http_destroy_server] passed NULL pointers for mandatory parameters");
    return NULL;
  }

  free_clients_group(server->clients); 
  free(server);
}
