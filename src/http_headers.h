#ifndef HEADERS_H_
#define HEADERS_H_
#include "includes.h"

typedef struct {
  char* v;
  size_t len;
} http_hdk;

typedef struct __http_hdv {
  char* v;
  size_t len;
  struct __http_hdv* next;
} http_hdv;

struct bucket {
  char state;
  http_hdk key;
  http_hdv* val;
};

typedef struct {
  size_t cap;
  size_t len;
  struct bucket* buckets;
} http_headers;

http_headers* http_headers_make(void);
int http_header_set(http_headers*, const char*, const char*);
http_hdv* http_header_get(http_headers*, const char*);
int http_header_remove(http_headers*, const char*);
int http_header_next(http_headers*, size_t*, http_hdk*, http_hdv**);
int http_header_reset(http_headers*);
int http_header_free(http_headers*);

#endif
