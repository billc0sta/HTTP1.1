#include "includes.h"
#include "client_info.h"
#include "request_info.h"
#include "request_parser.h"
#include <WinSock2.h>
#include <ws2tcpip.h>

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
		fprintf(stderr, "[main] bind() failed - %d.\n", WSAGetLastError());
		exit(1);
	}
	freeaddrinfo(binder);
	if (listen(server, 10) != 0) {
		fprintf(stderr, "[main] listen() failed - %d.\n", WSAGetLastError());
		exit(1);
	}
	if (request_info_init() != 0) {
		fprintf(stderr, "[main] request_info_init() failed.\n");
		exit(1); 
	}
	request_parser_init();
	struct client_group clients = make_client_group();
	while (1) {
		fd_set read;
		if (ready_clients(&clients, server, &read) != 0) {
			fprintf(stderr, "[main] ready_clients() failed.\n");
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
					parse_request(&client);
				}
			}
		}
	}

	request_parser_end(); 
	free_clients_group(&clients);
	WSACleanup();
	return 0;
}