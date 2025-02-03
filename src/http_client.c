#include "http_client.h"

http_client* http_client_new(http_constraints* constraints) {
  http_client* client = malloc(sizeof(http_client));
  if (!client) {
    HTTP_LOG(HTTP_LOGERR, "[http_client_new] malloc() failed.\n");
    return NULL;
  }
  memset(client, 0, sizeof(client));
  client->constraints = *constraints;
  client->conn        = conn_info_new(constraints);
  return client;
}

int http_client_free(http_client* client) {
  if (!client) {
    HTTP_LOG(HTTP_LOGERR, "[http_client_free] passed NULL pointers for mandatory parameters.\n");
    return HTTP_FAILURE;
  }
  if (conn_info_free(client->conn) == HTTP_FAILURE) {
    HTTP_LOG(HTTP_LOGERR, "[http_client_free] conn_info_free() failed.\n");
    return HTTP_FAILURE;
  }
  free(client);
  return HTTP_SUCCESS;
}

int http_client_connect(http_client* client, const char* ip, const char* port) {
  if (!client || !ip || !port) {
    HTTP_LOG(HTTP_LOGERR, "[http_client_connect] passed NULL pointers for mandatory parameters.\n");
    return HTTP_FAILURE;
  }

  struct addrinfo hints, *binder;
  struct conn_info* conn = client->conn;
  memset(&hints, 0, sizeof(hints));
  hints.ai_family = AF_INET;
  hints.ai_socktype = SOCK_STREAM;
  if (getaddrinfo(ip, port, &hints, &binder)) {
    HTTP_LOG(HTTP_LOGERR, "[http_client_connect] getaddrinfo() failed.\n");
    return HTTP_FAILURE;
  }
  conn->sockfd = socket(binder->ai_family, binder->ai_socktype, binder->ai_protocol);
  if (conn->sockfd == INVALID_SOCKET) {
    HTTP_LOG(HTTP_LOGERR, "[http_client_connect] socket() failed - %d.\n", GET_ERROR());
    return HTTP_FAILURE;
  }
  if (connect(conn->sockfd, binder->ai_addr, binder->ai_addrlen) == SOCKET_ERROR) {
    HTTP_LOG(HTTP_LOGERR, "[http_client_connect] connect() failed - %d.\n", GET_ERROR());
    return HTTP_FAILURE;
  }
  conn->addr   = *(struct sockaddr_in*)binder->ai_addr;
  conn->used   = 1; 
  freeaddrinfo(binder);
  return HTTP_SUCCESS;
}

int http_client_close(http_client* client) {
  if (!client) {
    HTTP_LOG(HTTP_LOGERR, "[http_client_close_connection] passed NULL pointers for mandatory parameters.\n");
    return HTTP_FAILURE;
  }
  struct conn_info* conn = client->conn;
  shutdown(conn->sockfd, 2); 
  CLOSE_SOCKET(conn->sockfd);
  conn->used = 0;
  return HTTP_SUCCESS;
}

http_request* http_client_get_request(http_client* client) {
  if (!client) {
    HTTP_LOG(HTTP_LOGERR, "[http_client_get_request] passed NULL pointers for mandatory parameters.\n");
    return NULL;
  }
  return &client->conn->request;
}

http_response* http_client_get_response(http_client* client) {
  if (!client) {
    HTTP_LOG(HTTP_LOGERR, "[http_client_get_response] passed NULL pointers for mandatory parameters.\n");
    return NULL;
  }
  return &client->conn->response;
}

int http_client_send(http_client* client) {
  if (!client) {
    HTTP_LOG(HTTP_LOGERR, "[http_client_send] passed NULL pointers for mandatory parameters.\n");
    return HTTP_FAILURE;
  }
}