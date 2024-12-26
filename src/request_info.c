#include "request_info.h"
#include "includes.h"

int make_request_info(struct request_info* req) {
	if (!req) {
		fprintf(stderr, "[make_request_info] passed NULL pointers for mandatory parameters.\n");
		return 1;
	}

	req->method = METHOD_NONE;
	req->resource = malloc(RESOURCE_BUFFLEN + 1);
	if (!req->resource) {
		fprintf(stderr, "[make_request_info] failed to allocate memory.\n");
		return 1;
	}
	req->headers = make_headers(); 
	if (!req->headers) {
		fprintf(stderr, "[make_request_info] hashmap_new() failed.\n");
		return 1;
	}
	req->resource[RESOURCE_BUFFLEN] = 0;
	req->version = HTTP_VERSION_NONE;
	req->body[BODY_BUFFLEN] = 0;
	return 0;
}

int free_request_info(struct request_info* req) {
	if (!req) {
		fprintf(stderr, "[free_request_info] passed NULL pointers for mandatory parameters.\n");
		return 1; 
	}
	free(req->resource);
	free_headers(req->headers); 
	return 0; 
}

int reset_request_info(struct request_info* req) {
	if (!req) {
		fprintf(stderr, "[reset_request_info] passed NULL pointers for mandatory parameters.\n");
		return 1;
	}
	reset_headers(req->headers);
	req->finished = 0;
	req->method   = METHOD_NONE;
	req->version  = HTTP_VERSION_NONE;
	req->body_len = 0;
	return 0;
}

int add_header_request_info(struct request_info* req, char* header, char* value) {
	if (!req || !header || !value) {
		fprintf(stderr, "[add_header_request_info] passed NULL pointers for mandatory parameters.\n");
		return 1;
	}
	set_header(req->headers, header, value);
}