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
		fprintf(stderr, "[make_request_info] failed.\n");
		return 1;
	}
	req->resource[RESOURCE_BUFFLEN] = 0;
	req->version = HTTP_VERSION_NONE;
	req->headers = NULL;
	req->headers_cap = 0;
	req->headers_len = 0;
	return 0;
}

int free_request_info(struct request_info* req) {
	if (!req) {
		fprintf(stderr, "[free_request_info] passed NULL pointers for mandatory parameters\n");
		return 1; 
	}
	free(req->resource);
}

int reset_request_info(struct request_info* req) {
	req->method  = METHOD_NONE;
	req->version = HTTP_VERSION_NONE;
	req->headers_len = 0;
	return 0;
}

int add_header(struct request_info* req, struct header hdr) {
	if (!req) {
		fprintf(stderr, "[add_header] passed NULL pointers for mandatory parameters.\n");
		return 1;
	}

	if (req->headers_len == req->headers_cap) {
		size_t new_cap = MAX(0, req->headers_cap * 2);
		struct header* new_data = realloc(req->headers, new_cap * sizeof(struct header));
		if (!new_data) {
			fprintf(stderr, "[add_header] failed to allocate memory.\n");
			return 1;
		}
		req->headers = new_data;
		req->headers_cap = new_cap;
	}

	req->headers[req->headers_len] = hdr;
	++req->headers_len;
	return 0; 
}