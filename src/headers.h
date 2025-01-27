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

http_headers* make_headers(void);
int set_header(http_headers*, const char*, const char*);
http_hdv* get_header(http_headers*, const char*);
int remove_header(http_headers*, const char*);
int next_header(http_headers*, size_t*, http_hdk*, http_hdv**);
int reset_headers(http_headers*);
int free_headers(http_headers*);

#endif
