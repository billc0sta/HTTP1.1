#include <WinSock2.h>
#include <ws2tcpip.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#define CLIENT_BUFFLEN 512
#define SELECT_TIMEOUT 5

int max_socket = 0;

struct Client {
	SOCKET             sockfd;
	struct sockaddr_in addr;
	socklen_t		   addrlen; 
	char               buffer[CLIENT_BUFFLEN + 1];
	int                bufflen;
	char               used;
	struct Request     request;
};

enum { METHOD_GET, METHOD_POST, METHOD_HEAD, METHOD_NONE};

struct Request {
	char method;
};

struct Clients {
	size_t         len;
	size_t         cap;
	struct Client* data;
};

int add_client(struct Clients*, SOCKET, struct sockaddr_in*, socklen_t);
int drop_client(struct Client*);
struct Clients make_clients();
int free_clients(struct Clients*);
int ready_clients(struct Clients*, fd_set*); 
int print_client_address(struct Client*);
int parse_request_part(struct Client*); 

int main() {
	WSADATA data;
	int res = 0;
	if ((res = WSAStartup(MAKEWORD(2, 2), &data)) != 0) {
		fprintf(stderr, "[main] WSAStartup() failed - %d.\n", res);
		exit(1);
	}

	struct addrinfo hints;
	memset(&hints, 0, sizeof(hints));
	hints.ai_family   = AF_INET;
	hints.ai_flags    = AI_PASSIVE;
	hints.ai_socktype = SOCK_STREAM;
	struct addrinfo* binder;
	getaddrinfo(0, "8080", &hints, &binder);
	SOCKET server = socket(binder->ai_family, binder->ai_socktype, binder->ai_protocol);
	if (server == INVALID_SOCKET) {
		fprintf(stderr, "[main] socket() failed - %d.\n", WSAGetLastError());
		exit(1);
	}
	max_socket = server;
	if (bind(server, binder->ai_addr, binder->ai_addrlen) != 0) {
		fprint(stderr, "[main] bind() failed - %d.\n", WSAGetLastError());
		exit(1);
	}
	if (listen(server, 10) != 0) {
		fprintf(stderr, "[main] listen() failed - %d.\n", WSAGetLastError());
		exit(1);
	}
	
	struct Clients clients = make_clients();
	while (1) {
		fd_set read;
		FD_ZERO(&read);
		FD_SET(server, &read);
		if (ready_clients(&clients, &read) == 1) {
			exit(1);
		}

		if (FD_ISSET(server, &read)) {
			struct sockaddr_in client_addr;
			int addrlen = sizeof(client_addr);
			SOCKET client_socket = accept(server, (struct sockaddr*)&client_addr, &addrlen);
			if (client_socket == INVALID_SOCKET) {
				fprintf(stderr, "[main] accept() failed - %d.\n", WSAGetLastError());
				exit(1);
			}
			add_client(&clients, client_socket, &client_addr, addrlen);
			printf("accepted a client.\n");
			print_client_address(&client_addr);
		}
		
		const size_t cap = clients.cap;
		for (size_t i = 0; i < cap; ++i) {
			struct Client* client = &clients.data[i];
			if (FD_ISSET(client->sockfd, &read)) {
				res = recv(client->sockfd, client->buffer + client->bufflen, CLIENT_BUFFLEN - client->bufflen, 0);
				if (res < 1) {
					printf("client disconnected unexpectedly.\n");
					print_client_address(&client);
					drop_client(&client);
				}

				else {
					client->bufflen += res;
					client->buffer[client->bufflen] = 0;
					char* q = strstr(client->buffer, "\r\n");
					if (q) {
						parse_request_part(&client);
						memcpy(client->buffer, q + 2, client->bufflen - (client->buffer - q) - 2);
					}
				}
			}
		}
	}

	free_clients(&clients);
	freeaddrinfo(binder);
	WSACleanup();
	return 0;
} 

struct Clients make_clients() {
	struct Clients clients;
	clients.cap  = 0;
	clients.len  = 0;
	clients.data = NULL;
	return clients;
}

int add_client(struct Clients* clients, SOCKET sockfd, struct sockaddr_in* addr, socklen_t addrlen) {
	if (!clients || !addr) {
		fprintf(stderr, "[add_client] passed NULL pointers for mandatory parameters.\n");
		return 1;
	}

	const size_t len = clients->len;
	const size_t cap = clients->cap;
	if (len < cap) {
		const struct Client* data = clients->data;
		for (size_t i = 0; i < cap; ++i) {
			if (data[i].used == 0) {
				struct Client* client = &data[i];
				client->addr    = *addr;
				client->sockfd  = sockfd;
				client->used    = 1;
				client->bufflen = 0;
				client->addrlen = addrlen;
				client->request.method = METHOD_NONE; 
				++clients->len;
				return 0;
			}
		}
	}

	else {
		size_t new_cap = cap == 0 ? 8 : cap * 2;
		struct Client* new_data = calloc(new_cap, sizeof(struct Client));
		if (new_data == NULL) {
			fprintf(stderr, "[add_client] realloc() failed.\n");
			return 1;
		}
		struct Client* old_data = clients->data;
		size_t counter = 0;
		for (size_t i = 0; i < cap; ++i) {
			if (old_data[i].used == 1) {
				struct Client* client = &new_data[counter++];
				client->addr    = old_data[i].addr;
				client->sockfd  = old_data[i].sockfd;
				client->used    = 1;
				client->bufflen = 0;
				client->addrlen = addrlen;
				client->request.method = METHOD_NONE;
			}
		}
		free(old_data);
		clients->data = new_data;
		clients->cap = new_cap;
		struct Client* client = &clients->data[counter++];
		client->addr = *addr;
		client->sockfd = sockfd;
		client->used = 1;
		clients->len = counter; 

		return 0;
	}
}

int drop_client(struct Client* client) {
	closesocket(client->sockfd);
	client->sockfd  = 0;
	client->bufflen = 0;
	client->used    = 0;
	return 0;
}

int free_clients(struct Clients* clients) {
	if (!clients) {
		fprintf(stderr, "[free_clients] passed NULL pointers for mandatory parameters.\n");
		return 1;
	}

	free(clients->data);
	clients->cap  = 0;
	clients->len  = 0;
	clients->data = NULL;
	return 0;
}

int ready_clients(struct Clients* clients, fd_set* read) {
	if (!clients || !read) {
		fprintf(stderr, "[ready_clients] passed NULL pointers for mandatory parameters.\n");
		return 1;
	}
	const size_t cap = clients->cap;
	const struct Client* data = clients->data;
	for (size_t i = 0; i < cap; ++i) {
		if (data[i].used == 1) {
			FD_SET(data[i].sockfd, &read);
		}
	}
	struct timeval timeout;
	timeout.tv_sec = SELECT_TIMEOUT;
	if (select(max_socket + 1, &read, NULL, NULL, &timeout) < 0) {
		fprintf(stderr, "[ready_clients] select() failed - %d.\n", WSAGetLastError());
		return 1;
	}
	return 0;
}

int print_client_address(struct Client* client) {
	static char address[100];
	getnameinfo((struct sockaddr*)&client->addr, client->addrlen, address, 100, NULL, 0, NI_NUMERICHOST);
	printf("the client's address: %s", address);
	return 0;
}