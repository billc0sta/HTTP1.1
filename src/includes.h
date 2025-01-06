#ifndef INCLUDES_H_
#define INCLUDES_H_

#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include "headers.h"
#define SELECT_SEC 5
#define SELECT_USEC 0
#define HTTP_VERSION_NONE 2
#define MIN(a, b) ((a < b) ? (a) : (b))
#define MAX(a, b) ((a > b) ? (a) : (b))

enum {
  HTTP_VERSION_1,
  HTTP_VERSION_1_1
}

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
#endif

#ifdef HTTP_DEBUG
#ifndef HTTP_LOGOUT 
#define HTTP_LOGOUT stdout
#ifndef HTTP_LOGERR
#define HTTP_LOGERR stderr 
#define HTTP_LOG(f, s) fprintf(f, s)
#else
#define HTTP_LOG(f, s) ;
#endif

#ifndef REQUEST_BODY_BUFFLEN
#define REQUEST_BODY_BUFFLEN (1024 * 1024)
#endif
#ifndef RESPONSE_BODY_BUFFLEN
#define RESPONSE_BODY_BUFFLEN (1024 * 1024)
#endif 
typedef uint32_t ipv4_t;

#endif


/* I will leave some usage notes here:
   - anything that isn't prefixed with http_ is only meant for internal use
 */
   
