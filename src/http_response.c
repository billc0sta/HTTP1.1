#include "http_response.h" 

struct status
{
    int code;
    const char* string;
};

struct status status_info[HTTP_STATUS_NONE] =
{
    [HTTP_STATUS_100] = {.code = 100, .string = "Continue" },
    [HTTP_STATUS_101] = {.code = 101, .string = "Switching Protocols" },
    [HTTP_STATUS_102] = {.code = 102, .string = "Processing" },
    [HTTP_STATUS_103] = {.code = 103, .string = "Early Hints" },
    [HTTP_STATUS_200] = {.code = 200, .string = "OK" },
    [HTTP_STATUS_201] = {.code = 201, .string = "Created" },
    [HTTP_STATUS_202] = {.code = 202, .string = "Accepted" },
    [HTTP_STATUS_203] = {.code = 203, .string = "Non-Authoritative Information" },
    [HTTP_STATUS_204] = {.code = 204, .string = "No Content" },
    [HTTP_STATUS_205] = {.code = 205, .string = "Reset Content" },
    [HTTP_STATUS_206] = {.code = 206, .string = "Partial Content" },
    [HTTP_STATUS_300] = {.code = 300, .string = "Multiple Choices" },
    [HTTP_STATUS_301] = {.code = 301, .string = "Moved Permanently" },
    [HTTP_STATUS_302] = {.code = 302, .string = "Found" },
    [HTTP_STATUS_303] = {.code = 303, .string = "See Other" },
    [HTTP_STATUS_304] = {.code = 304, .string = "Not Modified" },
    [HTTP_STATUS_305] = {.code = 305, .string = "Use Proxy" },
    [HTTP_STATUS_306] = {.code = 306, .string = "unused" },
    [HTTP_STATUS_307] = {.code = 307, .string = "Temporary Redirect" },
    [HTTP_STATUS_308] = {.code = 308, .string = "Permanent Redirect" },
    [HTTP_STATUS_400] = {.code = 400, .string = "Bad Request" },
    [HTTP_STATUS_401] = {.code = 401, .string = "Unauthorized" },
    [HTTP_STATUS_402] = {.code = 402, .string = "Payment Required" },
    [HTTP_STATUS_403] = {.code = 403, .string = "Forbidden" },
    [HTTP_STATUS_404] = {.code = 404, .string = "Not Found" },
    [HTTP_STATUS_405] = {.code = 405, .string = "Method Not Allowed" },
    [HTTP_STATUS_406] = {.code = 406, .string = "Not Acceptable" },
    [HTTP_STATUS_407] = {.code = 407, .string = "Proxy Authentication Required" },
    [HTTP_STATUS_408] = {.code = 408, .string = "Request Timeout" },
    [HTTP_STATUS_409] = {.code = 409, .string = "Conflict" },
    [HTTP_STATUS_410] = {.code = 410, .string = "Gone" },
    [HTTP_STATUS_411] = {.code = 411, .string = "Length Required" },
    [HTTP_STATUS_412] = {.code = 412, .string = "Precondition Failed" },
    [HTTP_STATUS_413] = {.code = 413, .string = "Content Too Large" },
    [HTTP_STATUS_414] = {.code = 414, .string = "URI Too Long" },
    [HTTP_STATUS_415] = {.code = 415, .string = "Unsupported Media Type" },
    [HTTP_STATUS_416] = {.code = 416, .string = "Range Not Satisfiable" },
    [HTTP_STATUS_417] = {.code = 417, .string = "Expectations Failed" },
    [HTTP_STATUS_418] = {.code = 418, .string = "I'm a Teapot" },
    [HTTP_STATUS_421] = {.code = 421, .string = "Misdirected Request" },
    [HTTP_STATUS_425] = {.code = 425, .string = "Too Early" },
    [HTTP_STATUS_426] = {.code = 426, .string = "Upgrade Required" },
    [HTTP_STATUS_428] = {.code = 428, .string = "Precondition Required" },
    [HTTP_STATUS_429] = {.code = 429, .string = "Too Many Requests" },
    [HTTP_STATUS_431] = {.code = 431, .string = "Request Header Fields Too Large" },
    [HTTP_STATUS_451] = {.code = 451, .string = "Unavailable For Legal Reasons" },
    [HTTP_STATUS_500] = {.code = 500, .string = "Internal Server Error" },
    [HTTP_STATUS_501] = {.code = 501, .string = "Not Implemented" },
    [HTTP_STATUS_502] = {.code = 502, .string = "Bad Gateway" },
    [HTTP_STATUS_503] = {.code = 503, .string = "Service Unavailable" },
    [HTTP_STATUS_504] = {.code = 504, .string = "Gateway Timeout" },
    [HTTP_STATUS_505] = {.code = 505, .string = "HTTP Version Not Supported" },
    [HTTP_STATUS_506] = {.code = 506, .string = "Variant Also Negotiates" },
    [HTTP_STATUS_510] = {.code = 510, .string = "Not Extended" },
    [HTTP_STATUS_511] = {.code = 511, .string = "Network Authentication Required" },
};


int http_response_make(http_response* res, http_constraints* constraints) {
  if (!res) {
    HTTP_LOG(HTTP_LOGERR, "[make_response_info] passed NULL pointers for mandatory parameters.\n");
    return HTTP_FAILURE;
  }
  res->headers = http_headers_make();
  if (!res->headers) {
    HTTP_LOG(HTTP_LOGERR, "[make_response_info] make_headers() failed.\n");
    return HTTP_FAILURE;
  }
  res->status        = HTTP_STATUS_NONE;
  res->body_string   = NULL;
  res->body_len      = 0;
  res->body_type     = BODYTYPE_NONE;
  res->state         = STATE_GOT_NOTHING;
  res->current_val   = NULL;
  res->headers_iter  = 0;
  res->sent          = 0; 
  res->send_key      = 1;
  res->constraints   = constraints; 
  return HTTP_SUCCESS; 
}

int http_response_free(http_response* response) {
  if (!response) {
    HTTP_LOG(HTTP_LOGERR, "[http_response_free] passed NULL pointers for mandatory parameters.\n");
    return HTTP_FAILURE;
  }
  http_header_free(response->headers);
}

int http_response_set_status(http_response* res, int status) {
  if (!res) {
    HTTP_LOG(HTTP_LOGERR, "[http_response_set_status] passed NULL pointers for mandatory parameters.\n");
    return HTTP_FAILURE;
  }

  if (status < 0 || status >= HTTP_STATUS_NONE) {
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
  res->body_len    = len;
  res->body_type   = BODYTYPE_STRING;
  char size[20];
  sprintf(size, "%zu", len);
  if (http_headers_set(res->headers, "Content-Length", size) == HTTP_FAILURE ||
      http_headers_set(res->headers, "Content-Type", "text/plain") == HTTP_FAILURE) {
    HTTP_LOG(HTTP_LOGERR, "[http_response_set_header] set_header() failed.\n");
    return HTTP_FAILURE;
  }
  return HTTP_SUCCESS;
}

int http_response_set_body_file(http_response* res, char* file_name) {
  if (!res || !file_name) {
    HTTP_LOG(HTTP_LOGERR, "[http_response_set_body_file] passed NULL pointers for mandatory parameters.\n");
    return HTTP_FAILURE;
  }
  char* dir;
  const char* public_folder = res->constraints->public_folder;
  if (*public_folder == 0) {
      dir = file_name;
  }
  else {
      size_t len = strlen(file_name) + strlen(public_folder);
      dir = (char*)malloc(len + 10);
      if (!dir) {
        HTTP_LOG(HTTP_LOGERR, "[http_response_set_body_file] failed to allocate memory.\n");
        return HTTP_FAILURE;
      }
      snprintf(dir, len, "%s/%s", public_folder, file_name);
  }
  res->body_file = fopen(dir, "rb");
  if (!res->body_file) {
    HTTP_LOG(HTTP_LOGERR, "[http_response_set_body_file] couldn't open file - %s.\n", dir);
    if (dir != file_name) free(dir);
    return HTTP_FAILURE; 
  }
  if (dir != file_name) free(dir);
  res->body_type = BODYTYPE_FILE; 
  if (http_headers_set(res->headers, "Transfer-Encoding", "Chunked") == HTTP_FAILURE) {
      HTTP_LOG(HTTP_LOGERR, "[http_response_set_body_file] set_header() failed.\n");
      return HTTP_FAILURE;
  }
  if (http_headers_get(res->headers, "Content-Type") == NULL) {
    if (http_headers_set(res->headers, "Content-Type", "application/octet-stream") == HTTP_FAILURE) {
      HTTP_LOG(HTTP_LOGERR, "[http_response_set_body_file] set_header() failed.\n");
      return HTTP_FAILURE;
    }
  }
  return HTTP_SUCCESS;
}

int http_response_set_header(http_response* res, const char* name, const char* value) {
  if (!res || !name || !value) {
    HTTP_LOG(HTTP_LOGERR, "[http_response_set_header] passed NULL pointers for mandatory parameters.\n");
    return HTTP_FAILURE;
  }
  if (http_headers_set(res->headers, name, value) == HTTP_FAILURE) {
    HTTP_LOG(HTTP_LOGERR, "[http_response_set_header] set_header() failed.\n");
    return HTTP_FAILURE;
  } 
  return HTTP_SUCCESS;
}

int http_response_status_code(int status) {
  if (status < 0 || status >= HTTP_STATUS_NONE) {
    HTTP_LOG(HTTP_LOGERR, "[http_response_status_info] invalid status code.\n");
    return 0;
  }
  return status_info[status].code;
}

const char* http_response_status_string(int status) {
  if (status < 0 || status >= HTTP_STATUS_NONE) {
    HTTP_LOG(HTTP_LOGERR, "[http_response_status_info] invalid status code.\n"); 
    return NULL;
  }
  return status_info[status].string; 
}

int http_response_reset(http_response* res) {
  if (!res) {
    HTTP_LOG(HTTP_LOGERR, "[make_response_info] passed NULL pointers for mandatory parameters.\n");
    return HTTP_FAILURE;
  }
  http_headers_reset(res->headers);
  res->status = HTTP_STATUS_NONE;
  res->body_string = NULL;
  res->body_len = 0;
  res->body_type = BODYTYPE_NONE;
  res->state = STATE_GOT_NOTHING;
  res->current_val = NULL;
  res->headers_iter = 0;
  res->sent = 0;
  res->send_key = 1;
  return HTTP_SUCCESS;
}