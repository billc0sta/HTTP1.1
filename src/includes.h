#ifndef INCLUDES_H_
#define INCLUDES_H_
#define _WIN32_WINNT 0x501

#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <assert.h> 
#include "headers.h"
#define SELECT_SEC 5
#define SELECT_USEC 0
#define MIN(a, b) ((a < b) ? (a) : (b))
#define MAX(a, b) ((a > b) ? (a) : (b))

enum {
  HTTP_VERSION_1,
  HTTP_VERSION_1_1,
  HTTP_VERSION_NONE
};

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#define CLOSE_SOCKET(s) closesocket(s)
#define GET_ERROR() WSAGetLastError()
#define SIN_ADDR sin_addr.S_un.S_addr 
#else
#include <sys/socket.h>
#include <sys/select.h>
#include <netdb.h>
#include <errno.h>
#define SOCKET int
#define CLOSE_SOCKET(s) close(s)
#define GET_ERROR() errno
#define SIN_ADDR sin_addr.s_addr
#define SOCKET_ERROR -1
#endif

#ifdef HTTP_DEBUG

static int HTTP_LOG(int fd, const char* format, ...) {
  va_list args;
  va_start(args, format);
  int res = vfprintf(fd, format, args);
  va_end(args);
  return res; 
}

#ifndef HTTP_LOGOUT 
#define HTTP_LOGOUT stdout
#endif
#ifndef HTTP_LOGERR
#define HTTP_LOGERR stderr
#endif
#else
#define HTTP_LOG(f, s, ...) ;
#endif

typedef uint32_t ipv4_t;
 
#define HTTP_FAILURE 1
#define HTTP_SUCCESS 0 

typedef struct {
  size_t request_max_body_len;
  size_t request_max_uri_len;
  size_t request_max_headers;
  size_t request_max_header_len;
  size_t recv_len;
  size_t send_len; 
} http_constraints;

enum {
  STATE_GOT_NOTHING,
  STATE_GOT_LINE,
  STATE_GOT_HEADERS, 
  STATE_GOT_ALL,
};

#endif
