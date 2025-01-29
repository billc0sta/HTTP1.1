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
  server->sockfd          = -1;
  server->request_handler = request_handler;
  server->addr            = *(struct sockaddr_in*)binder->ai_addr;
  server->constraints     = constraints ? *constraints : http_make_default_constraints();
  server->clients         = make_client_group(&server->constraints); 
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

int http_send_response(struct client_info* client, http_constraints* constraints) {
  http_response* res = &client->response;
  http_request* req  = &client->request;
  size_t sent = 0;
  size_t max_send = constraints->send_len;
  char* buffer = client->buffer;
  SOCKET sockfd = client->sockfd;
  int ret = 0;
  while (sent < max_send) {
    const char* status_info = http_response_status_info(res->status);
    if (status_info == NULL) {
      HTTP_LOG(HTTP_LOGERR, "[http_send_response] invalid status code.\n");
      return HTTP_FAILURE;
    } 
    if (res->state == STATE_GOT_NOTHING) {
      snprintf(buffer, CLIENT_BUFFER_LEN, "%d %s %s\r\n",
               res->status,
               status_info,
               req->version == HTTP_VERSION_1_1 ? "HTTP/1.1" : "HTTP/1.0");
      if ((ret = send(sockfd, buffer, strlen(buffer), 0)) == SOCKET_ERROR) {
        HTTP_LOG(HTTP_STDERR, "[http_send_response] send() failed - %d.\n", GET_ERROR());
        return HTTP_FAILURE; 
      } 
      res->state = STATE_GOT_LINE;
      if (next_header(res->headers, &res->headers_iter, &res->current_key, &res->current_val) == HTTP_FAILURE)
        res->state = STATE_GOT_HEADERS;
      sent += ret;
    }
    else if (res->state == STATE_GOT_LINE) {
      http_hdk* key = &res->current_key;
      http_hdv* val = res->current_val;
      if (res->send_key) {
        if (key->len == res->sent) {
          if (ret = send(sockfd, ": ", 2, 0) == SOCKET_ERROR) {
            HTTP_LOG(HTTP_STDERR, "[http_send_response] send() failed - %d.\n", GET_ERROR());
            return HTTP_FAILURE;
          }
          sent += ret;
          res->sent = 0;
          res->send_key = 0;
        }
        else {
          if ((ret = send(sockfd, key->v + res->sent, MIN(max_send - sent, key->len - res->sent), 0)) == SOCKET_ERROR) {
            HTTP_LOG(HTTP_STDERR, "[http_send_response] send() failed - %d.\n", GET_ERROR());
            return HTTP_FAILURE;
          }
          sent += ret;
          res->sent += ret; 
        }
      }
      else {
        if (val->len == res->sent) {
          if ((ret = send(sockfd, "\r\n", 2, 0)) == SOCKET_ERROR) {
            HTTP_LOG(HTTP_STDERR, "[http_send_response] send() failed - %d.\n", GET_ERROR());
            return HTTP_FAILURE;
          }
          sent += ret; 
          if (val->next)
            res->current_val = val->next; 
          else {
            if (next_header(res->headers, &res->headers_iter, &res->current_key, &res->current_val) == HTTP_FAILURE) {
              if ((ret = send(sockfd, "\r\n", 2, 0)) == SOCKET_ERROR) {
                HTTP_LOG(HTTP_STDERR, "[http_send_response] send() failed - %d.\n", GET_ERROR());
                return HTTP_FAILURE;
              }
              res->state = STATE_GOT_HEADERS; 
            }
          }
          res->send_key = 1;
          res->sent = 0;
          
        }
        if ((ret = send(sockfd, val->v, MIN(max_send - sent, val->len - res->sent), 0)) == SOCKET_ERROR) {
          HTTP_LOG(HTTP_STDERR, "[http_send_response] send() failed - %d.\n", GET_ERROR());
          return HTTP_FAILURE;
        }
        sent += ret;
        res->sent += ret;
      }
    }
    else if (res->state == STATE_GOT_HEADERS) {
      if (res->body_type == BODYTYPE_STRING) {
        if (res->string_len == res->sent) {
          res->state = STATE_GOT_ALL; 
          res->sent = 0;
        }
        if ((ret = send(sockfd, res->body_string + res->sent, MIN(max_send - sent, res->string_len - res->sent), 0)) == SOCKET_ERROR) {
          HTTP_LOG(HTTP_STDERR, "[http_send_response] send() failed - %d.\n", GET_ERROR());
          return HTTP_FAILURE;
        }
        sent += ret;
        res->sent += ret;
      }
      ret = fread(buffer, 1, CLIENT_BUFFER_LEN, res->body_file);
      if ((ret = (send(sockfd, buffer, ret, 0)) == SOCKET_ERROR)) {
        HTTP_LOG(HTTP_STDERR, "[http_send_response] send() failed - %d.\n", GET_ERROR());
        return HTTP_FAILURE;
      }
      sent += ret; 
      res->sent += ret;
    }
  } 
  return HTTP_SUCCESS;
}

int http_server_listen(http_server* server) {
  if (!server) {
    HTTP_LOG(HTTP_LOGERR, "[http_server_listen] passed NULL pointers for mandatory parameters");
    return HTTP_FAILURE;
  }

  int retval = HTTP_SUCCESS;
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
  struct client_group clients = make_client_group(&server->constraints); 
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
#ifdef HTTP_DEBUG
      print_client_address(client);
#endif
    }
	
    const size_t cap = clients.cap;
    for (size_t i = 0; i < cap; ++i) {
      struct client_info* client = &clients.data[i];
      if (FD_ISSET(client->sockfd, &read)) {
        int res = recv(client->sockfd, client->buffer + client->buff_len, CLIENT_BUFFER_LEN - client->buff_len, 0);
        if (res < 1) {
          HTTP_LOG(HTTP_LOGOUT, "client disconnected.\n");
#ifdef HTTP_DEBUG
          print_client_address(client);
#endif
          drop_client(client);
        }
        else {
          client->buff_len += res;
          if (parse_request(client, &server->constraints) == HTTP_FAILURE)
            server->error_handler(&client->request, &client->response); 
          if (client->request.state == STATE_GOT_ALL)
            server->request_handler(&client->request, &client->response);
          if (http_send_response(client, &server->constraints) == HTTP_FAILURE) {
            HTTP_LOG(HTTP_LOGERR, "[http_listen] http_send_response() failed.\n");
            goto fail; 
          }
        }
      }
    }
  }
  
  goto cleanup;
 fail:
  retval = HTTP_FAILURE;
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
    .request_max_body_len = 1024 * 1024 * 2,    /* 2MB                */
    .request_max_uri_len  = 2048,               /* 2KB: standard spec */
    .request_max_headers  = 24,                 /* arbitrary          */
    .request_max_header_len = 1024 * 1024 * 8,  /* 8MB                */
    .recv_len = 1024 * 1024,                    /* 1MB                */
    .send_len = 1024 * 1024,                    /* 1MB                */
  };
  return constraints;
}

