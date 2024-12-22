#ifndef REQUEST_PARSER_H_
#define REQUEST_PARSER_H_

#include "client_info.h"

int parse_request(struct client_info*);
int request_parser_init(void); 
int request_parser_end(void); 
#endif