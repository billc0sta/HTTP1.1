#include "client_info.h"

struct client_group make_client_group(http_constraints* constraints) {
  struct client_group clients;
  clients.cap      = 0;
  clients.len      = 0;
  clients.data     = NULL;
  clients.constraints = constraints;
  return clients;
}

int reset_client_info(struct client_info* client, http_constraints* constraints) {
  if (!client) {
    HTTP_LOG(HTTP_LOGERR, "[reset_client_info] passed NULL pointers for mandatory parameters.\n");
    return HTTP_FAILURE;
  }

  client->sockfd = INVALID_SOCKET;
  client->buff_len = 0;
  client->used = 0;
  if (client->request.headers)
    http_request_reset(&client->request, client->sockfd, &client->addr);
  else
    http_request_make(&client->request, client->sockfd, &client->addr, constraints); 
  if (client->response.headers)
    http_response_reset(&client->response);
  else
    http_response_make(&client->response, constraints);
  return HTTP_SUCCESS;
}

struct client_info* add_client(struct client_group* clients, SOCKET sockfd, struct sockaddr_in* addr) {
  if (!clients || !addr) {
    HTTP_LOG(HTTP_LOGERR, "[add_client] passed NULL pointers for mandatory parameters.\n");
    return NULL;
  }

  const size_t len = clients->len;
  const size_t cap = clients->cap;
  if (len < cap) {
    struct client_info* data = clients->data;
    for (size_t i = 0; i < cap; ++i) {
      if (data[i].used == 0) {
        struct client_info* client = &data[i];
        client->addr = *addr;
        client->sockfd = sockfd;
        client->used = 1;
        reset_client_info(client, clients->constraints);
        ++clients->len;
        return client;
      }
    }
  }

  else {
    size_t new_cap = cap == 0 ? 8 : cap * 2;
    struct client_info* new_data = calloc(new_cap, sizeof(struct client_info));
    if (new_data == NULL) {
      HTTP_LOG(HTTP_LOGERR, "[add_client] realloc() failed.\n");
      return NULL;
    }
    struct client_info* old_data = clients->data;
    size_t counter = 0;
    for (size_t i = 0; i < cap; ++i) {
      if (old_data[i].used == 1) {
        new_data[counter++] = old_data[i];
      }
    }
    free(old_data);
    
    clients->data = new_data;
    clients->cap  = new_cap;
    struct client_info* client = &clients->data[counter++];
    client->addr = *addr;
    client->sockfd = sockfd;
    client->used = 1;
    clients->len = counter;
    http_request_make(&client->request, client->sockfd, &client->addr, clients->constraints);
    http_response_make(&client->response, clients->constraints); 
    return client;
  }

  // unreachable 
  return NULL;
}

int drop_client(struct client_info* client) {
  if (!client) {
    HTTP_LOG(HTTP_LOGERR, "[drop_client] passed NULL pointers for mandatory parameters.\n");
    return HTTP_FAILURE;
  }

  closesocket(client->sockfd);
  client->sockfd = 0;
  client->buff_len = 0;
  client->used = 0;
  return HTTP_SUCCESS;
}

int free_client_info(struct client_info* client) {
  if (!client) {
    HTTP_LOG(HTTP_LOGERR, "[free_client_info] passed NULL pointers for mandatory parameters.\n");
    return HTTP_FAILURE;
  }
  http_request_free(&client->request);
  return HTTP_SUCCESS; 
}

int free_clients_group(struct client_group* clients) {
  if (!clients) {
    HTTP_LOG(HTTP_LOGERR, "[free_clients] passed NULL pointers for mandatory parameters.\n");
    return HTTP_FAILURE;
  }
  size_t cap = clients->cap;
  for (size_t i = 0; i < cap; ++i) {
    free_client_info(&clients->data[i]);
  }
  free(clients->data);
  clients->cap = 0;
  clients->len = 0;
  clients->data = NULL;
  return HTTP_SUCCESS;
}

int ready_clients(struct client_group* clients, SOCKET server_sockfd, fd_set* read) {
  if (!clients) {
    HTTP_LOG(HTTP_LOGERR, "[ready_clients] passed NULL pointers for mandatory parameters.\n");
    return HTTP_FAILURE;
  }
  const size_t cap = clients->cap;
  const struct client_info* data = clients->data;
  FD_ZERO(read);
  FD_SET(server_sockfd, read); 
  fd_set wr;
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
  struct timeval timeout;
  timeout.tv_sec = SELECT_SEC;
  timeout.tv_usec = SELECT_USEC;
  if (select(max_socket + 1, read, &wr, NULL, &timeout) < 0) {
    HTTP_LOG(HTTP_LOGERR, "[ready_clients] select() failed - %d.\n", GET_ERROR());
    return HTTP_FAILURE;
  }
  return HTTP_SUCCESS;
}

#ifdef HTTP_DEBUG
int print_client_address(struct client_info* client) {
	if (!client) {
		fprintf(stderr, "[print_client_address] passed NULL pointers for mandatory parameters.\n");
		return HTTP_FAILURE;
	}
    char address[100];
	getnameinfo((struct sockaddr*)&client->addr, sizeof(client->addr), address, 100, NULL, 0, NI_NUMERICHOST);
	printf("the client's address: %s\n", address);
	return HTTP_SUCCESS;
}
#endif
