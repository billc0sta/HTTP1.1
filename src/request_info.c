#include "request_info.h"
#include "includes.h"
#include "value.h"

int make_request_info(struct request_info* req) {
	if (!req) {
		fprintf(stderr, "[make_request_info] passed NULL pointers for mandatory parameters.\n");
		return 1;
	}

	req->method = METHOD_NONE;
	req->resource = malloc(RESOURCE_BUFFLEN + 1);
	if (!req->resource) {
		fprintf(stderr, "[make_request_info] failed.\n");
		return 1;
	}
	req->resource[RESOURCE_BUFFLEN] = 0;
	req->version = HTTP_VERSION_NONE;
	req->body[BODY_BUFFLEN] = 0;
	memset(req->headers, 0, sizeof(req->headers));
	return 0;
}

int free_request_info(struct request_info* req) {
	if (!req) {
		fprintf(stderr, "[free_request_info] passed NULL pointers for mandatory parameters.\n");
		return 1; 
	}
	for (int i = 0; i < HDR_NOT_SUPPORTED; ++i) {
		free_values(req->headers[i]);
	}
	free(req->resource);
	return 0; 
}

int reset_request_info(struct request_info* req) {
	if (!req) {
		fprintf(stderr, "[reset_request_info] passed NULL pointers for mandatory parameters.\n");
		return 1;
	} 
	req->method   = METHOD_NONE;
	req->version  = HTTP_VERSION_NONE;
	req->body_len = 0;
	return 0;
}

int add_value_request_info(struct request_info* req, struct value* val, int type) {
	if (!req) {
		fprintf(stderr, "[add_header_value] passed NULL pointers for mandatory parameters.\n");
		return 1;
	}
	if (!req->headers[type]) 
		req->headers[type] = val;
	else {
		val->next = req->headers[type];
		req->headers[type] = val;
	}
	return 0;
}