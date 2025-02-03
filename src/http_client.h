#ifndef HTTP_CLIENT_H_
#define HTTP_CLIENT_H_
#include "includes.h"
#include "conn_info.h"

typedef struct {
	struct conn_info* conn;
	http_constraints constraints;
} http_client;

http_client* http_client_new(http_constraints*);
int http_client_free(http_client*);
http_request* http_client_get_request(http_client*);
http_response* http_client_get_response(http_client*);
int http_client_connect(http_client*, const char*, const char*);
int http_client_close(http_client*);
int http_client_send(http_client*);

#endif