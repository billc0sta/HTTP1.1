#include "http.h"

static int dump_headers(struct headers* headers) {
  size_t iter = 0;
  struct value* values;
  char* name;
  while (next_header(headers, &iter, &name, &values) == 0) {
    for (struct value* curr = values; curr; curr = curr->next) {
      printf("-- {%s:%s}\n", name, curr->v);
    }
  }
  return 0;
}

void request_handler(http_request* request, http_response* response) {
  dump_headers(request->headers);
}

int main() {
  if (http_init()) {
    fprintf(stderr, "[main] http_init() failed.\n");
    exit(1); 
  }
  
  http_server* server = http_server_new("0.0.0.0", "80", request_handler);
  if (!server) {
    fprintf(stderr, "[main] http_server_new() failed.\n");
  }
  http_server_listen(server);
  
  http_server_free(server);
  http_quit();
  return 0;
}
