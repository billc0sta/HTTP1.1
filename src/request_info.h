#define RESOURCE_BUFFLEN 255

enum { 
	METHOD_GET,
	METHOD_POST,
	METHOD_HEAD,
	METHOD_NONE
};

enum {
	HDR_AIM,
	HDR_ACCEPT,
	HDR_ACCEPT_CHARSET,
	HDR_ACCEPT_DATATIME,
	HDR_ACCEPT_ENCODING,
	HDR_ACCEPT_LANGUAGE,
	HDR_ACCESS_CONTROL_REQUEST_METHOD,
	HDR_ACCESS_CONTROL_REQUEST_HEADER,
	HDR_AUTHORIZATION,
	HDR_CACHE_CONTROL,
	HDR_CONNECTION,
	HDR_CONTENT_ENCODING,
	HDR_CONTENT_LENGTH,
	HDR_CONTENT_MD5,
	HDR_CONTENT_TYPE,
	HDR_COOKIE,
	HDR_DATE,
	HDR_EXPECT,
	HDR_FORWARDED,
	HDR_FROM,
	HDR_HOST,
	HDR_HTTP2_SETTINGS,
	HDR_IF_MATCH,
	HDR_IF_MODIFIED_SINCE,
	HDR_IF_RANGE,
	HDR_IF_UNMODIFIED_SINCE,
	HDR_MAX_FORWARDS,
	HDR_ORIGIN,
	HDR_PREFER,
	HDR_PROXY_AUTHORIZATION,
	HDR_RANGE,
	HDR_REFERER,
	HDR_TE,
	HDR_TRAILER,
	HDR_TRANSFER_ENCODING,
	HDR_USER_AGENT,
	HDR_UPGRADE,
	HDR_VIA,
	HDR_WARNING,
	HDR_NONE
};

struct host_info { 
	ipv4_t ip;
	uint16_t port; 
};

struct header_field {
	char* name;
	struct header_value* value;
};

enum {
	HDRV_TEXT,
	HDRV_NUMBER,
	HDRV_BOOLEAN,
	HDRV_SPECIFIER,
	HDRV_DATE,
	HDRV_FIELD,
	HDRV_HOST,
	HDRV_LIST
};

struct header_value {
	int type;
	union {
		char* text;
		long   number;
		char   boolean;
		int    specifier;
		time_t date;
		struct header_field field;
		struct host_info host;
		struct header* list;
	} v;
};

struct header {
	int type;
	struct header_value value;
};

struct request_info {
	char method;
	char version;
	char* resource;
	char finished;
	struct header* headers;
	int headers_len; 
	int headers_cap;
};

int make_request_info(struct request_info* req);
int free_request_info(struct request_info* req);
int reset_request_info(struct request_info* req);