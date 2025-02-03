#include "http_server.h" 
#include "conn_info.h"
#include "parser.h"

int http_init(void) {
#ifdef _WIN32
  WSADATA data;
  int res = WSAStartup(MAKEWORD(2, 2), &data);
  if (res) {
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
  http_response_set_status(res, HTTP_STATUS_400);
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
  server->error_handler   = http_default_error_handler; 
  server->addr            = *(struct sockaddr_in*)binder->ai_addr;
  server->constraints     = constraints ? *constraints : http_make_default_constraints();
  server->conns           = conn_group_make(&server->constraints); 
  freeaddrinfo(binder);
  return server;
}

int http_server_free(http_server* server) {
  if (!server) {
    HTTP_LOG(HTTP_LOGERR, "[http_server_free] passed NULL pointers for mandatory parameters");
    return HTTP_FAILURE;
  }

  conn_group_free(&server->conns); 
  free(server);
  return HTTP_SUCCESS;
}

int http_send_response(struct conn_info* conn, http_constraints* constraints) {
  http_response* res = &conn->response;
  http_request* req  = &conn->request;
  size_t sent = 0;
  size_t max_send = constraints->send_len;
  char* buffer = conn->buffer;
  SOCKET sockfd = conn->sockfd;
  int ret = 0;
  char hex[20]; 
  while (sent < max_send && res->state != STATE_GOT_ALL) {
    if (res->state == STATE_GOT_NOTHING) {
        const char* status_string = http_response_status_string(res->status);
        int status_code = http_response_status_code(res->status);
        if (status_string == NULL) {
          HTTP_LOG(HTTP_LOGERR, "[http_send_response] invalid status code.\n");
          return HTTP_FAILURE;
        }
      snprintf(buffer, CONN_BUFF_LEN, "%s %d %s\r\n",
               req->version == HTTP_VERSION_1_1 ? "HTTP/1.1" : "HTTP/1.0",
               status_code,
               status_string);
      if ((ret = send(sockfd, buffer, (int)strlen(buffer), 0)) == SOCKET_ERROR) {
        HTTP_LOG(HTTP_LOGERR, "[http_send_response] send() failed - %d.\n", GET_ERROR());
        return HTTP_FAILURE; 
      } 
      res->state = STATE_GOT_LINE;
      if (http_headers_next(res->headers, &res->headers_iter, &res->current_key, &res->current_val) == HTTP_FAILURE)
        res->state = STATE_GOT_HEADERS;
      sent += ret;
    }
    else if (res->state == STATE_GOT_LINE) {
      http_hdk* key = &res->current_key;
      http_hdv* val = res->current_val;
      if (res->send_key) {
        if (key->len == res->sent) {
          if ((ret = send(sockfd, ": ", 2, 0)) == SOCKET_ERROR) {
            HTTP_LOG(HTTP_LOGERR, "[http_send_response] send() failed - %d.\n", GET_ERROR());
            return HTTP_FAILURE;
          }
          sent += ret;
          res->sent = 0;
          res->send_key = 0;
        }
        else {
          if ((ret = send(sockfd, key->v + res->sent, (int)MIN(max_send - sent, key->len - res->sent), 0)) == SOCKET_ERROR) {
            HTTP_LOG(HTTP_LOGERR, "[http_send_response] send() failed - %d.\n", GET_ERROR());
            return HTTP_FAILURE;
          }
          sent += ret;
          res->sent += ret; 
        }
      }
      else {
        if (val->len == res->sent) {
          if ((ret = send(sockfd, "\r\n", 2, 0)) == SOCKET_ERROR) {
            HTTP_LOG(HTTP_LOGERR, "[http_send_response] send() failed - %d.\n", GET_ERROR());
            return HTTP_FAILURE;
          }
          sent += ret; 
          if (val->next)
            res->current_val = val->next; 
          else {
            if (http_headers_next(res->headers, &res->headers_iter, &res->current_key, &res->current_val) == HTTP_FAILURE) {
              if ((ret = send(sockfd, "\r\n", 2, 0)) == SOCKET_ERROR) {
                HTTP_LOG(HTTP_LOGERR, "[http_send_response] send() failed - %d.\n", GET_ERROR());
                return HTTP_FAILURE;
              }
              res->state = STATE_GOT_HEADERS; 
            }
          }
          res->send_key = 1;
          res->sent = 0;
        }
        else {
          if ((ret = send(sockfd, val->v + res->sent, (int)MIN(max_send - sent, val->len - res->sent), 0)) == SOCKET_ERROR) {
            HTTP_LOG(HTTP_LOGERR, "[http_send_response] send() failed - %d.\n", GET_ERROR());
            return HTTP_FAILURE;
          }
          sent += ret;
          res->sent += ret;
        }
      }
    }
    else if (res->state == STATE_GOT_HEADERS) {
        if (res->body_type == BODYTYPE_STRING) {
          if (res->body_len == res->sent) {
            res->state = STATE_GOT_ALL;
            res->sent = 0;
          }
          else {
          if ((ret = send(sockfd, res->body_string + res->sent, (int)MIN(max_send - sent, res->body_len - res->sent), 0)) == SOCKET_ERROR) {
            HTTP_LOG(HTTP_LOGERR, "[http_send_response] send() failed - %d.\n", GET_ERROR());
            return HTTP_FAILURE;
          }
          sent += ret;
          res->sent += ret;
        } 
      }
      else {    
        int ret2;
        ret2 = (int)fread(buffer, 1, CONN_BUFF_LEN, res->body_file);
        ret = ret2;
        buffer[ret2] = 0; 
        if (ret2 == 0) {
            res->state = STATE_GOT_ALL;
            strcpy(hex, "0\r\n");
        }
        else 
            snprintf(hex, 20, "%.x\r\n", ret2);
        if ((ret = send(sockfd, hex, (int)strlen(hex), 0)) == SOCKET_ERROR) {
            HTTP_LOG(HTTP_LOGERR, "[http_send_response] send() failed - %d.\n", GET_ERROR());
            return HTTP_FAILURE;
        }
        sent += ret;
        res->sent += ret;
        if (ret2 > 0) {
          if ((ret = send(sockfd, buffer, ret2, 0)) == SOCKET_ERROR) {
            HTTP_LOG(HTTP_LOGERR, "[http_send_response] send() failed - %d.\n", GET_ERROR());
            return HTTP_FAILURE;
          }
          sent += ret2;
          res->sent += ret2;
        }
        if ((ret = send(sockfd, "\r\n", 2, 0)) == SOCKET_ERROR) {
            HTTP_LOG(HTTP_LOGERR, "[http_send_response] send() failed - %d.\n", GET_ERROR());
            return HTTP_FAILURE;
        }
        sent += 2;
        res->sent += 2;
      }
    }
  } 
  return HTTP_SUCCESS;
}

int http_validate_response(http_response* res) {
  if (res->status < 0 || res->status >= HTTP_STATUS_NONE) {
    HTTP_LOG(HTTP_LOGERR, "[http_validate_response] invalid status code - response handler is not set correctly.\n");
    return HTTP_FAILURE;
  }

  http_headers* headers = res->headers;
  http_hdv* length = http_headers_get(headers, "Content-Length");
  http_hdv* tren = http_headers_get(headers, "Transfer-Encoding");
  if (length) {
    if (tren) {
      HTTP_LOG(HTTP_LOGERR, "[http_validate_response] invalid headers - both 'Content-Length' and 'Transfer-Encoding' are set.\n");
      return HTTP_FAILURE;
    }
    size_t len = strtoul(length, 0, 10);
    if (len == 0) {
      HTTP_LOG(HTTP_LOGERR, "[http_validate_response] invalid headers - 'Content-Length' is set to 0 or non-number.\n");
      return HTTP_FAILURE;
    }
    if (res->body_len != 0 && len > res->body_len) {
      HTTP_LOG(HTTP_LOGERR, "[http_validate_response] invalid headers - 'Content-Length' > body_len");
      return HTTP_FAILURE;
    } 
    res->body_len = len;
    res->body_termination = BODYTERMI_LENGTH;
  }
  else {

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
  HTTP_LOG(HTTP_LOGOUT, "listening...\n");
  struct conn_group* conns = &server->conns; 
  while (1) {
    fd_set read;
    if (conn_group_wait(conns, server->sockfd, &read) == HTTP_FAILURE) {
      HTTP_LOG(HTTP_LOGERR, "[http_server_listen] conn_group_wait() failed.\n");
      goto fail; 
    }
    
    if (FD_ISSET(server->sockfd, &read)) {
      struct sockaddr_in conn_addr = { 0 };
      int addrlen = sizeof(conn_addr);
      SOCKET conn_socket = accept(server->sockfd, (struct sockaddr*)&conn_addr, &addrlen);
      if (conn_socket < 0) {
        HTTP_LOG(HTTP_LOGERR, "[http_server_listen] accept() failed - %d.\n", GET_ERROR());
        goto fail; 
      }
      struct conn_info* conn = conn_group_add(conns, conn_socket, &conn_addr);
      if (!conn) {
        HTTP_LOG(HTTP_LOGERR, "[http_server_listen] conn_group_add() failed.\n");
        goto fail;
      }
      HTTP_LOG(HTTP_LOGOUT, "accepted a client.\n");
#ifdef HTTP_DEBUG
      print_addr(&conn->addr);
#endif
    }
    
    const size_t cap = conns->cap;
    for (size_t i = 0; i < cap; ++i) {
      struct conn_info* conn = &conns->data[i];
      if (conn->used == 0) continue; 
      if (conn->request.state == STATE_GOT_ALL) {
        if (http_send_response(conn, &server->constraints) == HTTP_FAILURE) {
          HTTP_LOG(HTTP_LOGERR, "[http_listen] http_send_response() failed.\n");
          goto fail;
        }
        if (conn->response.state == STATE_GOT_ALL) {
            http_request_reset(&conn->request, conn->sockfd, &conn->addr);
            http_response_reset(&conn->response);
            conn->buff_len = 0;
        }
      }
      else if (FD_ISSET(conn->sockfd, &read)) {
        int res = recv(conn->sockfd, conn->buffer + conn->buff_len, (int)(CONN_BUFF_LEN - conn->buff_len), 0);
        if (res < 0) {
          HTTP_LOG(HTTP_LOGOUT, "client disconnected disgracefully.\n");
#ifdef HTTP_DEBUG
          print_addr(&conn->addr);
#endif
          conn_info_drop(conn);
        }
        else if (res > 0) {
          conn->buff_len += res;
          if (parse_request(conn, conn->buffer, &conn->buff_len, &server->constraints) == HTTP_FAILURE) {
            server->error_handler(&conn->request, &conn->response);
            if (http_validate_response(&conn->response) == HTTP_FAILURE) {
              HTTP_LOG(HTTP_LOGERR, "[http_server_listen] http_validate_response() failed.\n");
              goto fail;
            }
            conn->request.state = STATE_GOT_ALL; 
          }
          else if (conn->request.state == STATE_GOT_ALL) {
            server->request_handler(&conn->request, &conn->response);
            if (http_validate_response(&conn->response) == HTTP_FAILURE) {
                HTTP_LOG(HTTP_LOGERR, "[http_server_listen] http_validate_response() failed.\n");
                goto fail;
            }
          }
        }
        else {
          HTTP_LOG(HTTP_LOGOUT, "client disconnected gracefully.\n");
#ifdef HTTP_DEBUG
          print_addr(&conn->addr);
#endif
          conn_info_drop(conn);
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
    .public_folder = ""
  };
  return constraints;
}

