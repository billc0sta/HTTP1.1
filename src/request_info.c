#include "request_info.h"

int http_request_make(http_request* req, SOCKET client_socket, struct sockaddr_in *client_address, http_constraints* constraints) {
  if (!req) {
    HTTP_LOG(HTTP_LOGERR, "[make_request_info] passed NULL pointers for mandatory parameters.\n");
    return HTTP_FAILURE;
  }

  req->method = METHOD_NONE;
  req->resource = malloc(constraints->request_max_uri_len + 1);
  if (!req->resource) {
    HTTP_LOG(HTTP_LOGERR, "[make_request_info] malloc() failed.\n");
    return HTTP_FAILURE;
  }
  req->headers = make_headers(); 
  if (!req->headers) {
    free(req->resource);
    HTTP_LOG(HTTP_LOGERR, "[make_request_info] make_headers() failed.\n");
    return HTTP_FAILURE;
  }
  req->body = malloc(constraints->request_max_body_len + 1);
  if (!req->body) {
    free(req->resource);
    free_headers(req->headers);
    HTTP_LOG(HTTP_LOGERR, "[make_request_info] malloc() failed.\n");
    return HTTP_FAILURE;
  }
  req->uri_len = 0;
  req->body_len = 0;
  req->uri[constraints->request_max_uri_len] = 0;
  req->version = HTTP_VERSION_NONE;
  req->client_socket = client_socket;
  req->client_address = *client_address;
  req->state = STATE_GOT_NOTHING;
  req->body_termination = BODYTERMI_NONE;
  req->length = 0;
  req->chunk  = 0; 
  return HTTP_SUCCESS;
}

int http_request_free(http_request* req) {
  if (!req) {
    HTTP_LOG(HTTP_LOGERR, "[free_request_info] passed NULL pointers for mandatory parameters.\n");
    return HTTP_FAILURE; 
  }
  free(req->uri);
  free(req->body);
  free_headers(req->headers); 
  return HTTP_SUCCESS; 
}

int http_request_reset(http_request* req, SOCKET client_socket, struct sockaddr_in *client_address) {
  if (!req) {
    HTTP_LOG(HTTP_LOGERR, "[reset_request_info] passed NULL pointers for mandatory parameters.\n");
    return HTTP_FAILURE;
  }
  reset_headers(req->headers);
  req->state    = STATE_GOT_NOTHING;
  req->method   = METHOD_NONE;
  req->version  = HTTP_VERSION_NONE;
  req->body_len = 0;
  req->uri_len  = 0;
  req->client_socket    = client_socket;
  req->client_address   = *client_address;
  req->body_termination = BODYTERMI_NONE;
  req->length = 0;
  req->chunk  = 0; 
  return HTTP_SUCCESS;
}

int http_request_add_header(http_request* req, const char* header, const char* value) {
  if (!req || !header || !value) {
    HTTP_LOG(HTTP_LOGERR, "[add_header_request_info] passed NULL pointers for mandatory parameters.\n");
    return HTTP_FAILURE;
  }
  set_header(req->headers, header, value);
}
