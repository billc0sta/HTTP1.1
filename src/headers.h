#ifndef HEADERS_H_
#define HEADERS_H_
#include "includes.h"

struct value {
  char* v;
  int len;
  struct value* next;
};

struct bucket {
  char state;
  char* key;
  struct value* val;
};

struct headers {
  size_t cap;
  size_t len;
  struct bucket* buckets;
};

struct headers* make_headers(void);
int set_header(struct headers*, const char*, const char*);
struct value* get_header(struct headers*, const char*);
int remove_header(struct headers*, const char*);
int next_header(struct headers*, size_t*, char**, struct value**);
int reset_headers(struct headers*);
int free_headers(struct headers*);

#endif
