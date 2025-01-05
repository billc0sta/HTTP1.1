#include "request_info.h"
#include "includes.h"

int make_request_info(struct request_info* req, SOCKET client_socket, struct sockaddr_in *client_address) {
  if (!req) {
    http_log(stderr, "[make_request_info] passed NULL pointers for mandatory parameters.\n");
    return 1;
  }

  req->method = METHOD_NONE;
  req->resource = malloc(RESOURCE_BUFFLEN + 1);
  if (!req->resource) {
    http_log(stderr, "[make_request_info] failed to allocate memory.\n");
    return 1;
  }
  req->headers = make_headers(); 
  if (!req->headers) {
    http_log(stderr, "[make_request_info] hashmap_new() failed.\n");
    return 1;
  }
  req->resource_len = 0;
  req->resource[RESOURCE_BUFFLEN] = 0;
  req->version = HTTP_VERSION_NONE;
  req->body[BODY_BUFFLEN] = 0;
  req->client_socket = client_socket;
  req->client_address = *client_address;
  req->state = STATE_GOT_NOTHING;
  return 0;
}

int free_request_info(struct request_info* req) {
  if (!req) {
    http_log(stderr, "[free_request_info] passed NULL pointers for mandatory parameters.\n");
    return 1; 
  }
  free(req->resource);
  free_headers(req->headers); 
  return 0; 
}

int reset_request_info(struct request_info* req, SOCKET client_socket, struct sockaddr_in *client_address) {
  if (!req) {
    http_log(stderr, "[reset_request_info] passed NULL pointers for mandatory parameters.\n");
    return 1;
  }
  reset_headers(req->headers);
  req->state = STATE_GOT_NOTHING;
  req->method   = METHOD_NONE;
  req->version  = HTTP_VERSION_NONE;
  req->body_len = 0;
  req->request_len    = 0;
  req->client_socket  = client_socket;
  req->client_address = *client_address;
  return 0;
}

int add_header_request_info(struct request_info* req, char* header, char* value) {
  if (!req || !header || !value) {
    http_log(stderr, "[add_header_request_info] passed NULL pointers for mandatory parameters.\n");
    return 1;
  }
  set_header(req->headers, header, value);
}
