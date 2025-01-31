#ifdef _DEBUG
#define HTTP_DEBUG 1
#endif

#include "http.h"

static int dump_headers(http_headers* headers) {
  size_t iter = 0;
  http_hdv* values;
  http_hdk  name;
  while (next_header(headers, &iter, &name, &values) == 0) {
    for (http_hdv* curr = values; curr; curr = curr->next) {
      printf("-- {%s:%s}\n", name.v, curr->v);
    }
  }
  return 0;
}

void handler(http_request* request, http_response* response) {
  http_response_set_status(response, HTTP_STATUS_200);
  http_response_set_body(response, "Hello from Kudos.", 17);
}

int main() {
  if (http_init() != HTTP_SUCCESS) {
    return 1;
  }
  
  http_server* server = http_server_new("0.0.0.0", "80", handler, NULL);
  if (server) {
    http_server_listen(server); 
    http_server_free(server);
  }

  http_quit();

  return 0;
}
