#include "includes.h"

#ifdef HTTP_DEBUG

int print_addr(struct sockaddr_in* addr) {
  char address[100];
  getnameinfo((struct sockaddr*)addr, sizeof(addr), address, 100, NULL, 0, NI_NUMERICHOST);
  printf("the client's address: %s\n", address);
  return HTTP_SUCCESS;
}

int HTTP_LOG(FILE* file, const char* format, ...) {
  va_list args;
  va_start(args, format);
  int res = vfprintf(file, format, args);
  va_end(args);
  return res;
}

#endif

http_constraints http_constraints_make_default() {
	http_constraints constraints = {
	  .request_max_body_len = 1024 * 1024 * 2,    /* 2MB                */
	  .request_max_uri_len = 2048,               /* 2KB: standard spec */
	  .request_max_headers = 24,                 /* arbitrary          */
	  .request_max_header_len = 1024 * 1024 * 8,  /* 8MB                */
	  .recv_len = 1024 * 1024,                    /* 1MB                */
	  .send_len = 1024 * 1024,                    /* 1MB                */
	  .public_folder = ""
	};
	return constraints;
}
