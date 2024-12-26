#ifndef HEADERS_H_
#define HEADERS_H_
#include <stdint.h>

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

typedef struct {
	size_t cap;
	size_t len;
	struct bucket* buckets;
} headers;


headers* make_headers(void);
int set_header(headers*, char*, char*);
struct value* get_header(headers*, char*);
int remove_header(headers*, char*);
int next_header(headers*, size_t*, char**, struct value**);
int reset_headers(headers*);
int free_headers(headers*);

#endif