#include "client_info.h"
#include "includes.h"
#include <WinSock2.h>
#include <ws2tcpip.h>

int parse(struct client_info*); 

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
	if (bind(server, binder->ai_addr, binder->ai_addrlen) != 0) {
		fprint(stderr, "[main] bind() failed - %d.\n", WSAGetLastError());
		exit(1);
	}
	freeaddrinfo(binder);
	if (listen(server, 10) != 0) {
		fprintf(stderr, "[main] listen() failed - %d.\n", WSAGetLastError());
		exit(1);
	}
	struct client_group clients = make_clients();
	while (1) {
		fd_set read;
		if (ready_clients(&clients, server, &read) == 1) {
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
			add_client(&clients, client_socket, &client_addr);
			printf("accepted a client.\n");
			print_client_address(&client_addr);
		}
		
		const size_t cap = clients.cap;
		for (size_t i = 0; i < cap; ++i) {
			struct client_info* client = &clients.data[i];
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
					parse(&client);
				}
			}
		}
	}

	free_clients(&clients);
	WSACleanup();
	return 0;
} 

int parse(struct client_info* client) {
	struct request_info* req = &client->request;
	int len = 0;
	char* q     = client->buffer;
	char* start = q;
	char* end   = q + client->bufflen - 1;

	while (q <= end && strstr(q, "\r\n")) {
		if (req->method == METHOD_NONE) {

			q = strchr(q, ' ');
			if (!q)
				return 1;
			*q = 0;
			if (strcmp(start, "GET") == 0)
				req->method = METHOD_GET;
			else if (strcmp(start, "HEAD") == 0)
				req->method = METHOD_HEAD;
			else if (strcmp(start, "POST") == 0)
				req->method = METHOD_POST;
			else
				return 1;

			++q;
			start = q;
			if (q > end)
				return 1;
			if (*start != '/')
				return 1;
			q = strchr(q, ' ');
			if (!q)
				return 1;
			*q = 0;
			if ((len = strlen(start)) > RESOURCE_BUFFLEN)
				return 1;
			if (strstr(start, ".."))
				return 1;
			memcpy(req->resource, start, len);

			++q;
			start = q;
			if (q > end)
				return 1;
			q = strchr(q, ' ');
			if (!q)
				return 1;
			*q = 0;
			if (strcmp(start, "HTTP/1.0") == 0)
				req->version = HTTP_VERSION_1;
			else if (strcmp(start, "HTTP/1.1") == 0)
				req->version = HTTP_VERSION_1_1;
			else
				return 1;

			++q;
			if (q > end - 2)
				return 1;
			if (memcmp(q, "\r\n", 2) != 0)
				return 1;
			q += 2;
		}

		else {
			q = strchr(q, ':');
			if (!q)
				return 0;
			*q = 0;
			
		}
	}
	return 0;
}