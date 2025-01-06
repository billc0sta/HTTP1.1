#include "http.h" 
#include "client_info.h"
#include "request_parser.h"

int http_init(void) {
#ifdef _WIN32
  WSADATA data;
  int res;
  if (res = WSAStartup(MAKEWORD(2, 2), &data)) {
    fprintf(stderr, "[http_init] WSAStartup() failed - %d.\n", res);
    return 1;
  }
#endif
  return 0;
}

int http_quit(void) {  
#ifdef _WIN32
  if (WSACleanup())
    return 1;
#endif
  return 0;
}

void http_default_error_handler(http_request* request, http_response* response) {

}

http_server* http_server_new(const char* ip, const char* port, req_handler request_handler) {
  if (!ip || !port || !request_handler) {
    HTTP_LOG(HTTP_LOGERR, "[http_server_new] passed NULL pointers for mandatory parameters");
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
  
  http_server* server = malloc(sizeof(http_server));
  if (!server) {
    freeaddrinfo(binder);
    HTTP_LOG(HTTP_LOGERR, "[http_server_new] failed to allocate memory.\n");
    return NULL
  }
  
  server->ip      = ntohl(((struct sockaddr_in*)&binder->ai_addr)->SIN_ADDR);
  server->port    = ntohs(((struct sockaddr_in*)&binder->ai_addr)->sin_port);
  server->clients = make_client_group();
  server->sockfd  = -1;
  server->request_handler = request_handler;
  freeaddrinfo(binder);
  return server;
}

int http_server_free(http_server* server) {
  if (!server) {
    HTTP_LOG(HTTP_LOGERR, "[http_server_free] passed NULL pointers for mandatory parameters");
    return 1;
  }

  free_clients_group(server->clients); 
  free(server);
  return 0;
}

int http_server_listen(http_server* server) {
  if (!server) {
    HTTP_LOG(HTTP_LOGERR, "[http_server_listen] passed NULL pointers for mandatory parameters");
    return 1;
  }

  int retval = 0;
  server->sockfd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
  if (server->sockfd < 0) {
    HTTP_LOG(HTTP_LOGERR, "[http_server_listen] socket() failed - %d.\n", GET_ERROR());
    return 1;
  }
  if (bind(server_sockfd, binder->ai_addr, (int)binder->ai_addrlen)) {
    HTTP_LOG(HTTP_LOGERR, "[http_server_listen] bind() failed - %d.\n", GET_ERROR());
    goto fail; 
  }
  if (listen(server_sockfd, 10)) {
    HTTP_LOG(HTTP_LOGERR, "[http_server_listen] listen() failed - %d.\n", GET_ERROR());
    goto fail; 
  }
  HTTP_LOG(HTTP_LOGOUT, "listening..."); 

  http_response* response = malloc(sizeof(http_response));
  if (!response) {
    HTTP_LOG(HTTP_LOGERR, "[http_server_listen] failed to allocate memory.\n");
    goto fail; 
  }
  while (1) {
    fd_set read;
    if (ready_clients(&clients, server->sockfd, &read)) {
      HTTP_LOG(HTTP_LOGERR, "[http_server_listen] ready_clients() failed.\n");
      goto fail; 
    }
    
    if (FD_ISSET(server->sockfd, &read)) {
      struct sockaddr_in client_addr;
      int addrlen = sizeof(client_addr);
      SOCKET client_socket = accept(server_sockfd, (struct sockaddr*)&client_addr, &addrlen);
      if (client_socket < 0) {
        HTTP_LOG(HTTP_LOGERR, "[http_server_listen] accept() failed - %d.\n", GET_ERROR());
        goto fail; 
      }
      struct client_info* client = add_client(&clients, client_socket, &client_addr);
      if (!client) {
        HTTP_LOG(HTTP_LOGERR, "[http_server_listen] add_client() failed.\n");
        goto fail;
      }
      HTTP_LOG(HTTP_LOGOUT, "accepted a client.\n");
      print_client_address(client);
    }
	
    const size_t cap = clients.cap;
    for (size_t i = 0; i < cap; ++i) {
      struct client_info* client = &clients.data[i];
      if (FD_ISSET(client->sockfd, &read)) {
        res = recv(client->sockfd, client->buffer + client->bufflen, CLIENT_BUFFLEN - client->bufflen, 0);
        if (res < 1) {
          HTTP_LOG(HTTP_LOGOUT, "client disconnected.\n");
          print_client_address(client);
          drop_client(client);
        }
        else {
          client->bufflen += res;
          if (parse_request(client))
            server->error_handler(&client->request); 
          if (client->request.state == STATE_GOT_ALL)
            server->request_handler(&client->request); 
        }
      }
    }
  }
  
  goto cleanup;
 fail:
  retval = 1;
 cleanup: 
  CLOSE_SOCKET(server->sockfd);
  server->sockfd = -1;
  return retval; 
}

int http_server_set_error_handler(http_server* server, req_handler error_handler) {
  if (!server || error_handler) {
    HTTP_LOG(HTTP_LOGERR, "[http_server_set_error_handler] passed NULL pointers for mandatory parameters");
    return 1;
  }

  server->error_handler = error_handler; 
  return 0; 
}
