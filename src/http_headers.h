#ifndef HTTP_HEADERS_H_
#define HTTP_HEADERS_H_
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
int http_headers_set(http_headers*, const char*, const char*);
http_hdv* http_headers_get(http_headers*, const char*);
int http_headers_remove(http_headers*, const char*);
int http_headers_next(http_headers*, size_t*, http_hdk*, http_hdv**);
int http_headers_reset(http_headers*);
int http_headers_free(http_headers*);

#endif
