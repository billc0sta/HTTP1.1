#include "conn_info.h"

struct conn_group conn_group_make(http_constraints* constraints) {
  struct conn_group conns = { 0 };
  conns.cap      = 0;
  conns.len      = 0;
  conns.data     = NULL;
  conns.constraints = constraints;
  return conns;
}

struct conn_info* conn_info_new(http_constraints* constraints) {
  struct conn_info* conn = malloc(sizeof(struct conn_info));
  if (!conn) {
    HTTP_LOG(HTTP_LOGERR, "[conn_info_new] malloc() failed.\n");
    return NULL;
  }
  memset(conn, 0, sizeof(conn));
  conn_info_reset(conn, constraints);
  return conn;
}

int conn_info_reset(struct conn_info* conn, http_constraints* constraints) {
  if (!conn) {
    HTTP_LOG(HTTP_LOGERR, "[reset_conn_info] passed NULL pointers for mandatory parameters.\n");
    return HTTP_FAILURE;
  }

  conn->sockfd = INVALID_SOCKET;
  conn->buff_len = 0;
  conn->used = 0;
  if (conn->request.headers) {
    http_request_reset(&conn->request, conn->sockfd, &conn->addr);
  }
  else {
    if (http_request_make(&conn->request, conn->sockfd, &conn->addr, constraints) == HTTP_FAILURE) {
      HTTP_LOG(HTTP_LOGERR, "[reset_conn_info] http_request_make failed.\n");
      return HTTP_FAILURE;
    }
  }
  if (conn->response.headers) {
    http_response_reset(&conn->response);
  }
  else {
    if (http_response_make(&conn->response, constraints) == HTTP_FAILURE) {
      HTTP_LOG(HTTP_LOGERR, "[reset_conn_info] http_response_make failed.\n");
      return HTTP_FAILURE;
    }
  }
  return HTTP_SUCCESS;
}

struct conn_info* conn_group_add(struct conn_group* conns, SOCKET sockfd, struct sockaddr_in* addr) {
  if (!conns || !addr) {
    HTTP_LOG(HTTP_LOGERR, "[add_conn] passed NULL pointers for mandatory parameters.\n");
    return NULL;
  }

  const size_t len = conns->len;
  const size_t cap = conns->cap;
  if (len < cap) {
    struct conn_info* data = conns->data;
    for (size_t i = 0; i < cap; ++i) {
      if (data[i].used == 0) {
        struct conn_info* conn = &data[i];
        conn_info_reset(conn, conns->constraints);
        conn->addr = *addr;
        conn->sockfd = sockfd;
        conn->used = 1;
        ++conns->len;
        return conn;
      }
    }
  }

  else {
    size_t new_cap = cap == 0 ? 8 : cap * 2;
    struct conn_info* new_data = calloc(new_cap, sizeof(struct conn_info));
    if (new_data == NULL) {
      HTTP_LOG(HTTP_LOGERR, "[add_conn] realloc() failed.\n");
      return NULL;
    }
    struct conn_info* old_data = conns->data;
    size_t counter = 0;
    for (size_t i = 0; i < cap; ++i) {
      if (old_data[i].used == 1) {
        new_data[counter++] = old_data[i];
      }
    }
    free(old_data);
    
    conns->data = new_data;
    conns->cap  = new_cap;
    struct conn_info* conn = &conns->data[counter++];
    conn->addr = *addr;
    conn->sockfd = sockfd;
    conn->used = 1;
    conns->len = counter;
    if (http_request_make(&conn->request, conn->sockfd, &conn->addr, conns->constraints) == HTTP_FAILURE) {
      HTTP_LOG(HTTP_LOGERR, "[add_conn] http_request_make failed.\n");
      return NULL;
    }
    if (http_response_make(&conn->response, conns->constraints) == HTTP_FAILURE) {
      HTTP_LOG(HTTP_LOGERR, "[add_conn] http_response_make failed.\n");
      return NULL;
    }
    return conn;
  }

  // unreachable 
  return NULL;
}

int conn_info_drop(struct conn_info* conn) {
  if (!conn) {
    HTTP_LOG(HTTP_LOGERR, "[drop_conn] passed NULL pointers for mandatory parameters.\n");
    return HTTP_FAILURE;
  }

  closesocket(conn->sockfd);
  conn->sockfd = 0;
  conn->buff_len = 0;
  conn->used = 0;
  return HTTP_SUCCESS;
}

int _conn_info_free(struct conn_info* conn) {
  if (conn->request.headers) {
    if (http_request_free(&conn->request) == HTTP_FAILURE) {
      HTTP_LOG(HTTP_LOGERR, "[conn_info_free] http_request_free() failed.\n");
      return HTTP_FAILURE;
    }
  }
  if (conn->response.headers) {
    if (http_response_free(&conn->response) == HTTP_FAILURE) {
      HTTP_LOG(HTTP_LOGERR, "[conn_info_free] http_response_free() failed.\n");
      return HTTP_FAILURE;
    }
  }
  return HTTP_SUCCESS;
}

int conn_info_free(struct conn_info* conn) {
  if (!conn) {
    HTTP_LOG(HTTP_LOGERR, "[free_conn_info] passed NULL pointers for mandatory parameters.\n");
    return HTTP_FAILURE;
  }
  _conn_info_free(conn);
  free(conn);
  return HTTP_SUCCESS; 
}

int conn_group_free(struct conn_group* conns) {
  if (!conns) {
    HTTP_LOG(HTTP_LOGERR, "[free_conns] passed NULL pointers for mandatory parameters.\n");
    return HTTP_FAILURE;
  }
  size_t cap = conns->cap;
  for (size_t i = 0; i < cap; ++i) {
    _conn_info_free(&conns->data[i]);
  }
  free(conns->data);
  conns->cap = 0;
  conns->len = 0;
  conns->data = NULL;
  return HTTP_SUCCESS;
}

int conn_group_wait(struct conn_group* conns, SOCKET server_sockfd, fd_set* read) {
  if (!conns) {
    HTTP_LOG(HTTP_LOGERR, "[ready_conns] passed NULL pointers for mandatory parameters.\n");
    return HTTP_FAILURE;
  }
  const size_t cap = conns->cap;
  const struct conn_info* data = conns->data;
  FD_ZERO(read);
  FD_SET(server_sockfd, read); 
  fd_set wr = { 0 };
  FD_ZERO(&wr);
  int max_socket = (int)server_sockfd;
  for (size_t i = 0; i < cap; ++i) {
    if (data[i].used == 1) {
      SOCKET sockfd = data[i].sockfd;
      if (data[i].request.state == STATE_GOT_NOTHING)
        FD_SET(sockfd, read);
      else
        FD_SET(sockfd, &wr); 
      if (sockfd > max_socket) max_socket = (int)sockfd;
    }
  }
  struct timeval timeout = { 0 };
  timeout.tv_sec = SELECT_SEC;
  timeout.tv_usec = SELECT_USEC;
  if (select(max_socket + 1, read, &wr, NULL, &timeout) < 0) {
    HTTP_LOG(HTTP_LOGERR, "[ready_conns] select() failed - %d.\n", GET_ERROR());
    return HTTP_FAILURE;
  }
  return HTTP_SUCCESS;
}