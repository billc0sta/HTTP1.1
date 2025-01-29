#include "response_info.h" 

const char* status_string[HTTP_STATUS_NONE] =
  {
    [HTTP_STATUS_100] = "Continue",
    [HTTP_STATUS_101] = "Switching Protocols",
    [HTTP_STATUS_102] = "Processing",
    [HTTP_STATUS_103] = "Early Hints",
    [HTTP_STATUS_200] = "OK",
    [HTTP_STATUS_201] = "Created",
    [HTTP_STATUS_202] = "Accepted",
    [HTTP_STATUS_203] = "Non-Authoritative Information",
    [HTTP_STATUS_204] = "No Content",
    [HTTP_STATUS_205] = "Reset Content",
    [HTTP_STATUS_206] = "Partial Content",
    [HTTP_STATUS_300] = "Multiple Choices",
    [HTTP_STATUS_301] = "Moved Permanently",
    [HTTP_STATUS_302] = "Found",
    [HTTP_STATUS_303] = "See Other",
    [HTTP_STATUS_304] = "Not Modified",
    [HTTP_STATUS_305] = "Use Proxy",
    [HTTP_STATUS_306] = "unused",
    [HTTP_STATUS_307] = "Temporary Redirect",
    [HTTP_STATUS_308] = "Permanent Redirect",
    [HTTP_STATUS_400] = "Bad Request",
    [HTTP_STATUS_401] = "Unauthorized",
    [HTTP_STATUS_402] = "Payment Required",
    [HTTP_STATUS_403] = "Forbidden",
    [HTTP_STATUS_404] = "Not Found",
    [HTTP_STATUS_405] = "Method Not Allowed",
    [HTTP_STATUS_406] = "Not Acceptable",
    [HTTP_STATUS_407] = "Proxy Authentication Required",
    [HTTP_STATUS_408] = "Request Timeout",
    [HTTP_STATUS_409] = "Conflict",
    [HTTP_STATUS_410] = "Gone",
    [HTTP_STATUS_411] = "Length Required",
    [HTTP_STATUS_412] = "Precondition Failed",
    [HTTP_STATUS_413] = "Content Too Large",
    [HTTP_STATUS_414] = "URI Too Long",
    [HTTP_STATUS_415] = "Unsupported Media Type",
    [HTTP_STATUS_416] = "Range Not Satisfiable",
    [HTTP_STATUS_417] = "Expectations Failed",
    [HTTP_STATUS_418] = "I'm a Teapot",
    [HTTP_STATUS_421] = "Misdirected Request",
    [HTTP_STATUS_425] = "Too Early",
    [HTTP_STATUS_426] = "Upgrade Required",
    [HTTP_STATUS_428] = "Precondition Required",
    [HTTP_STATUS_429] = "Too Many Requests",
    [HTTP_STATUS_431] = "Request Header Fields Too Large",
    [HTTP_STATUS_451] = "Unavailable For Legal Reasons",
    [HTTP_STATUS_500] = "Internal Server Error",
    [HTTP_STATUS_501] = "Not Implemented",
    [HTTP_STATUS_502] = "Bad Gateway",
    [HTTP_STATUS_503] = "Service Unavailable",
    [HTTP_STATUS_504] = "Gateway Timeout",
    [HTTP_STATUS_505] = "HTTP Version Not Supported",
    [HTTP_STATUS_506] = "Variant Also Negotiates",
    [HTTP_STATUS_510] = "Not Extended",
    [HTTP_STATUS_511] = "Network Authentication Required",
  };

int http_response_make(http_response* res) {
  if (!res) {
    HTTP_LOG(HTTP_LOGERR, "[make_response_info] passed NULL pointers for mandatory parameters.\n");
    return HTTP_FAILURE;
  }
  res->headers = make_headers();
  if (!res->headers) {
    HTTP_LOG(HTTP_LOGERR, "[make_response_info] make_headers() failed.\n");
    return HTTP_FAILURE;
  }
  res->version       = HTTP_VERSION_NONE;
  res->status        = HTTP_STATUS_NONE;
  res->body_string   = NULL;
  res->string_len    = 0;
  res->body_type     = BODYTYPE_NONE;
  res->file_name     = NULL;
  res->state         = STATE_GOT_NOTHING;
  res->current_val   = NULL;
  res->headers_iter  = 0;
  res->sent          = 0; 
  res->send_key      = 0;
  return HTTP_SUCCESS; 
}

int http_response_set_version(http_response* res, int version) {
  if (!res) {
    HTTP_LOG(HTTP_LOGERR, "[http_response_set_version] passed NULL pointers for mandatory parameters.\n");
    return HTTP_FAILURE;
  }

  if (version != HTTP_VERSION_1 && version != HTTP_VERSION_1_1) {
    HTTP_LOG(HTTP_LOGERR, "[http_response] invalid arguments - this version is invalid or unsupported.\n");
    return HTTP_FAILURE;
  }
  res->version = version; 
  return HTTP_SUCCESS;
}

int http_response_set_status(http_response* res, int status) {
  if (!res) {
    HTTP_LOG(HTTP_LOGERR, "[http_response_set_status] passed NULL pointers for mandatory parameters.\n");
    return HTTP_FAILURE;
  }

  if (!(status >= HTTP_STATUS_100 && status < HTTP_STATUS_NONE)) {
    HTTP_LOG(HTTP_LOGERR, "[http_response_set_status] invalid arguments - invalid or unsupported status code.\n");
    return HTTP_FAILURE;
  }
  res->status = status; 
  return HTTP_SUCCESS;
}

int http_response_set_body(http_response* res, const unsigned char* bytes, size_t len) {
  if (!res) {
    HTTP_LOG(HTTP_LOGERR, "[http_response_set_body] passed NULL pointers for mandatory parameters.\n");
    return HTTP_FAILURE;
  }
  res->body_string = bytes;
  res->string_len  = len;
  res->body_type   = BODYTYPE_STRING;
  return HTTP_SUCCESS;
}

int http_response_set_body_file(http_response* res, const char* file_name) {
  if (!res || !file_name) {
    HTTP_LOG(HTTP_LOGERR, "[http_response_set_body_file] passed NULL pointers for mandatory parameters.\n");
    return HTTP_FAILURE;
  }
  res->file_name = file_name;
  res->body_type = BODYTYPE_FILE; 
  return HTTP_SUCCESS;
}

int http_response_set_header(http_response* res, const char* name, const char* value) {
  if (!res || !name || !value) {
    HTTP_LOG(HTTP_LOGERR, "[http_response_set_header] passed NULL pointers for mandatory parameters.\n");
    return HTTP_FAILURE;
  }
  if (set_header(res->headers, name, value) == HTTP_FAILURE) {
    HTTP_LOG(HTTP_LOGERR, "[http_response_set_header] set_header() failed.\n");
    return HTTP_FAILURE;
  } 
  return HTTP_SUCCESS;
}

const char* http_response_status_info(int status_code) {
  if (status_code < 0 || status_code >= HTTP_STATUS_NONE) {
    HTTP_LOG(HTTP_STDERR, "[http_response_status_info] invalid status code.\n")
    return NULL;
  }
  return status_string[status_code]; 
}
