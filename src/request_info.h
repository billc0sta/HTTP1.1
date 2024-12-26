#ifndef REQUEST_INFO_H_
#define REQUEST_INFO_H_

#include "includes.h"
#define RESOURCE_BUFFLEN 255
#define BODY_BUFFLEN (1024 * 1024 * 2)

enum {
	METHOD_GET,
	METHOD_POST,
	METHOD_HEAD,
	METHOD_PUT,
	METHOD_DELETE,
	METHOD_CONNECT,
	METHOD_OPTIONS,
	METHOD_TRACE,
	METHOD_PATCH,
	METHOD_NONE
};

struct request_info {
	char method;
	char version;
	char* resource;
	char finished;
	char body[BODY_BUFFLEN + 1];
	int body_len;
	headers* headers;
};

int make_request_info(struct request_info*);
int free_request_info(struct request_info*);
int reset_request_info(struct request_info*);
int add_header_request_info(struct request_info* req, char* header, char* value);
#endif