#include "http_request.h"

int http_request_make(http_request* req, SOCKET conn_socket, struct sockaddr_in *conn_address, http_constraints* constraints) {
  if (!req) {
    HTTP_LOG(HTTP_LOGERR, "[make_request_info] passed NULL pointers for mandatory parameters.\n");
    return HTTP_FAILURE;
  }

  req->method = METHOD_NONE;
  req->uri = malloc(constraints->request_max_uri_len + 1);
  if (!req->uri) {
    HTTP_LOG(HTTP_LOGERR, "[make_request_info] malloc() failed.\n");
    return HTTP_FAILURE;
  }
  req->headers = http_headers_make(); 
  if (!req->headers) {
    free(req->uri);
    HTTP_LOG(HTTP_LOGERR, "[make_request_info] make_headers() failed.\n");
    return HTTP_FAILURE;
  }
  req->body = malloc(constraints->request_max_body_len + 1);
  if (!req->body) {
    free(req->uri);
    http_headers_free(req->headers);
    HTTP_LOG(HTTP_LOGERR, "[make_request_info] malloc() failed.\n");
    return HTTP_FAILURE;
  }
  req->uri_len          = 0;
  req->body_len         = 0;
  req->uri[constraints->request_max_uri_len] = 0;
  req->version          = HTTP_VERSION_NONE;
  req->conn_socket      = conn_socket;
  req->conn_address     = *conn_address;
  req->state            = STATE_GOT_NOTHING;
  req->body_termination = BODYTERMI_NONE;
  req->length           = 0;
  req->chunk            = 0; 
  return HTTP_SUCCESS;
}

int http_request_free(http_request* req) {
  if (!req) {
    HTTP_LOG(HTTP_LOGERR, "[free_request_info] passed NULL pointers for mandatory parameters.\n");
    return HTTP_FAILURE; 
  }
  free(req->uri);
  free(req->body);
  http_headers_free(req->headers); 
  return HTTP_SUCCESS; 
}

int http_request_reset(http_request* req, SOCKET conn_socket, struct sockaddr_in *conn_address) {
  if (!req) {
    HTTP_LOG(HTTP_LOGERR, "[reset_request_info] passed NULL pointers for mandatory parameters.\n");
    return HTTP_FAILURE;
  }
  http_headers_reset(req->headers);
  req->state    = STATE_GOT_NOTHING;
  req->method   = METHOD_NONE;
  req->version  = HTTP_VERSION_NONE;
  req->body_len = 0;
  req->uri_len  = 0;
  req->conn_socket    = conn_socket;
  req->conn_address   = *conn_address;
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
  http_headers_set(req->headers, header, value);
  return HTTP_SUCCESS;
}
