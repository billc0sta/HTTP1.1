#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#define CLOSE_SOCKET(s) closesocket(s)
#define GET_ERROR() WSAGetLastError()
#else
#include <sys/socket.h>
#include <sys/select.h>
#include <netdb.h>
#include <errno.h>
#define SOCKET int
#define CLOSE_SOCKET(s) close(s)
#define GET_ERROR() errno
#endif

#include "includes.h"
#include "client_info.h"

int main() {
#ifdef _WIN32
	WSADATA data;
	int res;
	if (res = WSAStartup(MAKEWORD(2, 2), &data)) {
		fprintf(stderr, "[main] WSAStartup() failed - %d.\n", res);
		exit(1);
	}
#endif
	struct addrinfo hints;
	struct addrinfo* binder;
	memset(&hints, 0, sizeof(hints));
	hints.ai_family   = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags    = AI_PASSIVE;
	getaddrinfo(0, "80", &hints, &binder);
	SOCKET server_sockfd = socket(binder->ai_family, binder->ai_socktype, binder->ai_protocol);
	if (server_sockfd == INVALID_SOCKET) {
		fprintf(stderr, "[main] socket() failed - %d.\n", GET_ERROR());
		exit(1);
	}
	if (bind(server_sockfd, binder->ai_addr, (int)binder->ai_addrlen)) {
		fprintf(stderr, "[main] bind() failed - %d.\n", GET_ERROR());
		exit(1);
	}
	if (listen(server_sockfd, 10)) {
		fprintf(stderr, "[main] listen() failed - %d.\n", GET_ERROR());
		exit(1);
	}
	freeaddrinfo(binder);
	printf("listening...\n");

	struct client_group clients = make_client_group();
	while (1) {
		fd_set read;
		if (ready_clients(&clients, server_sockfd, &read)) {
			fprintf(stderr, "[main] ready_clients() failed.\n");
			exit(1);
		}

		if (FD_ISSET(server_sockfd, &read)) {
			struct sockaddr_in client_addr;
			int addrlen = sizeof(client_addr);
			SOCKET client_socket = accept(server_sockfd, (struct sockaddr*)&client_addr, &addrlen);
			if (client_socket == INVALID_SOCKET) {
				fprintf(stderr, "[main] accept() failed - %d.\n", GET_ERROR());
				exit(1);
			}
			struct client_info* client = add_client(&clients, client_socket, &client_addr);
			if (!client) {
				fprintf(stderr, "[main] add_client() failed.\n");
				exit(1);
			}
			printf("accepted a client.\n");
			print_client_address(client);
		}
	
		const size_t cap = clients.cap;
		for (size_t i = 0; i < cap; ++i) {
			struct client_info* client = &clients.data[i];
			if (FD_ISSET(client->sockfd, &read)) {
				res = recv(client->sockfd, client->buffer, CLIENT_BUFFLEN, 0);
				if (res < 1) {
					printf("client disconnected.\n");
					print_client_address(client);
					drop_client(client);
				}
				else {
					printf("%.*s", res, client->buffer);
				}
			}
		}
	}

#ifdef _WIN32
	WSACleanup();
#endif
	CLOSE_SOCKET(server_sockfd);
	return 0;
}