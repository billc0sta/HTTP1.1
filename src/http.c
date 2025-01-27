#include "http.h" 
#include "client_info.h"
#include "request_parser.h"

int http_init(void) {
#ifdef _WIN32
  WSADATA data;
  int res;
  if (res = WSAStartup(MAKEWORD(2, 2), &data)) {
    HTTP_LOG(HTTP_LOGERR, "[http_init] WSAStartup() failed - %d.\n", res);
    return HTTP_FAILURE;
  }
#endif
  return HTTP_SUCCESS;
}

int http_quit(void) {  
#ifdef _WIN32
  if (WSACleanup())
    return HTTP_FAILURE;
#endif
  return HTTP_SUCCESS;
}

void http_default_error_handler(http_request* req, http_response* res) {
  http_response_set_status(res, 400);
}

http_server* http_server_new(const char* ip, const char* port, request_handler request_handler, http_constraints* constraints) {
  if (!ip || !port || !request_handler) {
    HTTP_LOG(HTTP_LOGERR, "[http_server_new] passed NULL pointers for mandatory parameters");
    return NULL;
  }
  
  if (strcmp(ip, "0.0.0.0") == 0)
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
    return NULL; 
  }
  
  server->ip              = ntohl(((struct sockaddr_in*)&binder->ai_addr)->SIN_ADDR);
  server->port            = ntohs(((struct sockaddr_in*)&binder->ai_addr)->sin_port);
  server->clients         = make_client_group();
  server->sockfd          = -1;
  server->request_handler = request_handler;
  server->addr            = *(struct sockaddr_in*)binder->ai_addr;
  server->constraints     = constraints ? *constraints : http_make_default_constraints(); 
  freeaddrinfo(binder);
  return server;
}

int http_server_free(http_server* server) {
  if (!server) {
    HTTP_LOG(HTTP_LOGERR, "[http_server_free] passed NULL pointers for mandatory parameters");
    return HTTP_FAILURE;
  }

  free_clients_group(&server->clients); 
  free(server);
  return HTTP_SUCCESS;
}

int send_response(struct client_info* client) {
  char* buffer = client->buffer;
  struct headers* headers = client->response.headers;
  
  struct header_value* val;
  struct header_key key;
  size_t iter; 
  while (next_header(headers, &iter, &key, &val) == HTTP_SUCCESS) {
    send(client->sockfd, key->v, key->len, 0);
    while (val) {
      send(client->sockfd, val->v, val->len, 0);
      val = val->next; 
    }
  }
}

int http_server_listen(http_server* server) {
  if (!server) {
    HTTP_LOG(HTTP_LOGERR, "[http_server_listen] passed NULL pointers for mandatory parameters");
    return HTTP_FAILURE;
  }

  int retval = 0;
  server->sockfd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
  if (server->sockfd < 0) {
    HTTP_LOG(HTTP_LOGERR, "[http_server_listen] socket() failed - %d.\n", GET_ERROR());
    return HTTP_FAILURE;
  }
  if (bind(server->sockfd, (struct sockaddr*)&server->addr, (int)sizeof(server->addr))) {
    HTTP_LOG(HTTP_LOGERR, "[http_server_listen] bind() failed - %d.\n", GET_ERROR());
    goto fail; 
  }
  if (listen(server->sockfd, 10)) {
    HTTP_LOG(HTTP_LOGERR, "[http_server_listen] listen() failed - %d.\n", GET_ERROR());
    goto fail; 
  }
  HTTP_LOG(HTTP_LOGOUT, "listening...");
  struct client_group clients = make_client_group(); 
  while (1) {
    fd_set read;
    if (ready_clients(&clients, server->sockfd, &read)) {
      HTTP_LOG(HTTP_LOGERR, "[http_server_listen] ready_clients() failed.\n");
      goto fail; 
    }
    
    if (FD_ISSET(server->sockfd, &read)) {
      struct sockaddr_in client_addr;
      int addrlen = sizeof(client_addr);
      SOCKET client_socket = accept(server->sockfd, (struct sockaddr*)&client_addr, &addrlen);
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
        int res = recv(client->sockfd, client->buffer + client->bufflen, CLIENT_BUFFLEN - client->bufflen, 0);
        if (res < 1) {
          HTTP_LOG(HTTP_LOGOUT, "client disconnected.\n");
#ifdef HTTP_DEBUG
          print_client_address(client);
#endif
          drop_client(client);
        }
        else {
          client->bufflen += res;
          if (parse_request(client))
            server->error_handler(&client->request, &client->response); 
          if (client->request.state == STATE_GOT_ALL)
            server->request_handler(&client->request, &client->response);
          send_response(client);
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

int http_server_set_error_handler(http_server* server, request_handler error_handler) {
  if (!server || error_handler) {
    HTTP_LOG(HTTP_LOGERR, "[http_server_set_error_handler] passed NULL pointers for mandatory parameters");
    return HTTP_FAILURE;
  }

  server->error_handler = error_handler; 
  return HTTP_SUCCESS; 
}

http_constraints http_make_default_constraints() {
  http_constraints constraints = {
    .client_buffer_len = 1024;                  /* 1MB                */
    .request_max_body_len = 1024 * 1024 * 2;    /* 2MB                */
    .request_max_uri_len  = 2048;               /* 2KB: standard spec */
    .request_max_headers  = 24;                 /* arbitrary          */
    .request_max_headers_len = 1024 * 1024 * 8; /* 8MB                */
    .recv_len = 1024 * 1024;                    /* 1MB                */
    .send_len = 1024 * 1024;                    /* 1MB                */
  };
  return constraints;
}

