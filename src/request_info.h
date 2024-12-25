#ifndef REQUEST_INFO_H_
#define REQUEST_INFO_H_

#include "includes.h"
#define RESOURCE_BUFFLEN 255
#define BODY_BUFFLEN (1024 * 1024 * 2)

enum { 
	METHOD_GET,
	METHOD_POST,
	METHOD_HEAD,
	METHOD_NONE
};

struct request_info {
	char method;
	char version;
	char finished;
	char* resource;
	char body[BODY_BUFFLEN + 1];
	int body_len; 
};

int make_request_info(struct request_info*);
int free_request_info(struct request_info*);
int reset_request_info(struct request_info*);
#endif