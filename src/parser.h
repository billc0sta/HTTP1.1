#ifndef HTTP_PARSER_H_
#define HTTP_PARSER_H_

#include "conn_info.h"
#include "includes.h"

int parse_request(http_request*, char*, size_t*, http_constraints*);
#endif
