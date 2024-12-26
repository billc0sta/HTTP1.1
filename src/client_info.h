#ifndef CLIENT_INFO_H_
#define CLIENT_INFO_H_

#include <WinSock2.h>
#include <ws2tcpip.h>
#include "request_info.h"
#define CLIENT_BUFFLEN 512

struct client_info {
	SOCKET              sockfd;
	struct sockaddr_in  addr;
	socklen_t		    addrlen;
	char                buffer[CLIENT_BUFFLEN + 1];
	int                 bufflen;
	char                used;
	struct request_info request;
};

struct client_group {
	size_t         len;
	size_t         cap;
	struct client_info* data;
};

struct client_info* add_client(struct client_group*, SOCKET, struct sockaddr_in* s);
int drop_client(struct client_info*);
struct client_group make_client_group();
int free_clients_group(struct client_group*);
int ready_clients(struct client_group*, SOCKET, fd_set*);
int reset_client_info(struct client_info*);
int print_client_address(struct client_info*);

#endif