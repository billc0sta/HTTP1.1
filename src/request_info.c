#include "request_info.h"

int make_request_info(struct request_info* req, SOCKET client_socket, struct sockaddr_in *client_address) {
  if (!req) {
    HTTP_LOG(HTTP_LOGERR, "[make_request_info] passed NULL pointers for mandatory parameters.\n");
    return HTTP_FAILURE;
  }

  req->method = METHOD_NONE;
  req->resource = malloc(RESOURCE_BUFFLEN + 1);
  if (!req->resource) {
    HTTP_LOG(HTTP_LOGERR, "[make_request_info] failed to allocate memory.\n");
    return HTTP_FAILURE;
  }
  req->headers = make_headers(); 
  if (!req->headers) {
    HTTP_LOG(HTTP_LOGERR, "[make_request_info] hashmap_new() failed.\n");
    return HTTP_FAILURE;
  }
  req->resource_len = 0;
  req->resource[RESOURCE_BUFFLEN] = 0;
  req->version = HTTP_VERSION_NONE;
  req->body[REQUEST_BODY_BUFFLEN] = 0;
  req->client_socket = client_socket;
  req->client_address = *client_address;
  req->state = STATE_GOT_NOTHING;
  req->body_termination = BODYTERMI_NONE;
  req->length = 0;
  req->chunk  = 0; 
  return HTTP_SUCCESS;
}

int free_request_info(struct request_info* req) {
  if (!req) {
    HTTP_LOG(HTTP_LOGERR, "[free_request_info] passed NULL pointers for mandatory parameters.\n");
    return HTTP_FAILURE; 
  }
  free(req->resource);
  free_headers(req->headers); 
  return HTTP_SUCCESS; 
}

int reset_request_info(struct request_info* req, SOCKET client_socket, struct sockaddr_in *client_address) {
  if (!req) {
    HTTP_LOG(HTTP_LOGERR, "[reset_request_info] passed NULL pointers for mandatory parameters.\n");
    return HTTP_FAILURE;
  }
  reset_headers(req->headers);
  req->state    = STATE_GOT_NOTHING;
  req->method   = METHOD_NONE;
  req->version  = HTTP_VERSION_NONE;
  req->body_len = 0;
  req->resource_len     = 0;
  req->client_socket    = client_socket;
  req->client_address   = *client_address;
  req->body_termination = BODYTERMI_NONE;
  req->length = 0;
  req->chunk  = 0; 
  return HTTP_SUCCESS;
}

int add_header_request_info(struct request_info* req, const char* header, const char* value) {
  if (!req || !header || !value) {
    HTTP_LOG(HTTP_LOGERR, "[add_header_request_info] passed NULL pointers for mandatory parameters.\n");
    return HTTP_FAILURE;
  }
  set_header(req->headers, header, value);
}
