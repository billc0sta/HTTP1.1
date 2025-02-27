#ifndef HTTP_RESPONSE_H_
#define HTTP_RESPONSE_H_
#include "includes.h" 

enum {
  BODYTYPE_FILE,
  BODYTYPE_STRING,
  BODYTYPE_NONE,
};

enum {
  HTTP_STATUS_100,
  HTTP_STATUS_101,
  HTTP_STATUS_102,
  HTTP_STATUS_103,
  HTTP_STATUS_200,
  HTTP_STATUS_201,
  HTTP_STATUS_202,
  HTTP_STATUS_203,
  HTTP_STATUS_204,
  HTTP_STATUS_205,
  HTTP_STATUS_206,
  HTTP_STATUS_300,
  HTTP_STATUS_301,
  HTTP_STATUS_302,
  HTTP_STATUS_303,
  HTTP_STATUS_304,
  HTTP_STATUS_305,
  HTTP_STATUS_306,
  HTTP_STATUS_307,
  HTTP_STATUS_308,
  HTTP_STATUS_400,
  HTTP_STATUS_401,
  HTTP_STATUS_402,
  HTTP_STATUS_403,
  HTTP_STATUS_404,
  HTTP_STATUS_405,
  HTTP_STATUS_406,
  HTTP_STATUS_407,
  HTTP_STATUS_408,
  HTTP_STATUS_409,
  HTTP_STATUS_410,
  HTTP_STATUS_411,
  HTTP_STATUS_412,
  HTTP_STATUS_413,
  HTTP_STATUS_414,
  HTTP_STATUS_415,
  HTTP_STATUS_416,
  HTTP_STATUS_417,
  HTTP_STATUS_418,
  HTTP_STATUS_421,
  HTTP_STATUS_425,
  HTTP_STATUS_426,
  HTTP_STATUS_427,
  HTTP_STATUS_428,
  HTTP_STATUS_429,
  HTTP_STATUS_431,
  HTTP_STATUS_451,
  HTTP_STATUS_500,
  HTTP_STATUS_501,
  HTTP_STATUS_502,
  HTTP_STATUS_503,
  HTTP_STATUS_504,
  HTTP_STATUS_505,
  HTTP_STATUS_506,
  HTTP_STATUS_510,
  HTTP_STATUS_511,
  HTTP_STATUS_NONE
};

typedef struct {
  int status;
  http_headers* headers;
  const unsigned char* body_string;
  size_t body_len;
  FILE* body_file;
  int body_type;

  // internal use
  char state;
  http_hdk current_key;
  http_hdv* current_val;
  size_t headers_iter;
  size_t sent; 
  int send_key;
  http_constraints* constraints; 
  int body_termination;
} http_response;

int http_response_make(http_response*, http_constraints*);
int http_response_set_status(http_response*, int);
int http_response_set_body(http_response*, const unsigned char*, size_t);
int http_response_set_body_file(http_response*, char* file_name); 
int http_response_set_header(http_response*, const char*, const char*);
const char* http_response_status_string(int);
int http_response_status_code(int);
int http_response_reset(http_response*);
int http_response_free(http_response*);
#endif
