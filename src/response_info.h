#ifndef RESPONSE_INFO_H_
#define RESPONSE_INFO_H_
#include "includes.h" 

enum {
  BODYTYPE_FILE,
  BODYTYPE_STRING
};

struct response_info {
  int version;
  int status;
  char body[RESPONSE_BODY_BUFFLEN + 1];
  size_t body_len;
  struct headers* headers;
  int body_type;
  FILE* file; 
};

int http_response_set_version(struct response_info*, int);
int http_response_set_status(struct response_info*, int);
int http_response_set_body(struct response_info*, const unsigned char*, size_t);
int http_response_set_body_file(struct response_info*, const unsigned char*, FILE*); 
int http_response_set_header(struct response_info*, const char*, const char*);
#endif
