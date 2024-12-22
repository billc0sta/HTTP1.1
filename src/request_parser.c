#include "request_parser.h "
#include "includes.h"
#include "client_info.h"
#include "request_info.h"
#include "value.h"

static hashmap* _supported_headers;
static hashmap* _supported_media; 
static char* _lowered_string;
typedef struct value*(*value_parser) (char*, int);
value_parser parsers[HDR_NOT_SUPPORTED];

struct value* parse_text(char*, int);
struct value* parse_number(char*, int);
struct value* parse_specifier(char*, int);
struct value* parse_date(char*, int);
struct value* parse_field(char*, int);
struct value* parse_host(char*, int);
struct value* parse_list(char*, int);
struct value* parse_options(char*, int);
static int insensitive_string_cmp(const char*, const char*);

static struct value* parse_date(char* str, int type) {
	struct vdate date;
	if (strlen(str) < 29)
		return NULL;

	char* begin = str;
	str = strchr(str, ' ');
	if (!str)
		return NULL;
	*str++ = 0;
	if (strcmp(begin, "Sat") == 0)
		date.day = DAY_SAT;
	else if (strcmp(begin, "Sun") == 0)
		date.day = DAY_SUN;
	else if (strcmp(begin, "Mon") == 0)
		date.day = DAY_MON;
	else if (strcmp(begin, "Tue") == 0)
		date.day = DAY_TUE;
	else if (strcmp(begin, "Wed") == 0)
		date.day = DAY_WED;
	else if (strcmp(begin, "Thu") == 0)
		date.day = DAY_THU;
	else if (strcmp(begin, "Fri") == 0)
		date.day = DAY_FRI;
	else
		return NULL;

	if (*str != ',' || *(str+1) != ' ')
		return NULL;
	str += 2;

	begin = str;
	str = strchr(str, ' ');
	if (!str)
		return NULL;
	*str++ = 0;
	
	if (strlen(begin) != 2)
		return NULL;

	date.month_day = atoi(begin);
	if (date.month_day < 1 || date.month_day > 31)
		return NULL;

	begin = str;
	str = strchr(str, ' ');
	if (!str)
		return NULL;
	*str++ = 0;

	if (strcmp(begin, "Jan") == 0)
		date.month = MONTH_JAN;
	else if (strcmp(begin, "Feb") == 0)
		date.month = MONTH_FEB;
	else if (strcmp(begin, "Mar") == 0)
		date.month = MONTH_MAR;
	else if (strcmp(begin, "Apr") == 0)
		date.month = MONTH_APR;
	else if (strcmp(begin, "May") == 0)
		date.month = MONTH_MAY;
	else if (strcmp(begin, "Jun") == 0)
		date.month = MONTH_JUN;
	else if (strcmp(begin, "Jul") == 0)
		date.month = MONTH_JUL;
	else if (strcmp(begin, "Aug") == 0)
		date.month = MONTH_AUG;
	else if (strcmp(begin, "Sep") == 0)
		date.month = MONTH_SEP;
	else if (strcmp(begin, "Oct") == 0)
		date.month = MONTH_OCT;
	else if (strcmp(begin, "Nov") == 0)
		date.month = MONTH_NOV;
	else if (strcmp(begin, "Dec") == 0)
		date.month = MONTH_DEC;

	begin = str;
	str = strchr(str, ' ');
	if (!str)
		return NULL;
	*str++ = 0;
	if (strlen(begin) != 4)
		return NULL;

	date.year = atoi(begin);
	if (date.year < 1991 || date.year > 2100)
		return NULL;

	begin = str;
	str = strchr(str, ':');
	if (!str)
		return NULL;
	*str++ = 0;
	if (strlen(begin) != 2)
		return NULL;

	date.hour = atoi(begin);
	if (date.hour > 23)
		return NULL;
	
	begin = str;
	str = strchr(str, ':');
	if (!str)
		return NULL;
	*str++ = 0;
	if (strlen(begin) != 2)
		return NULL;

	date.minute = atoi(begin);
	if (date.minute > 59)
		return NULL;
	
	begin = str;
	str = strchr(str, ' ');
	if (!str)
		return NULL;
	*str++ = 0;
	if (strlen(begin) != 2)
		return NULL;

	date.second = atoi(begin);
	if (date.second > 59)
		return NULL;

	if (strcmp(str, "GMT") != 0)
		return NULL;

	return make_date_value(&date);
}

static struct value* parse_specifier(char* str, int type) {
	int spec = -1;
	switch (type) {
	case HDR_ACCESS_CONTROL_REQUEST_METHOD: {
		if (strcmp(str, "GET") == 0)
			spec = METHOD_GET;
		else if (strcmp(str, "HEAD") == 0)
			spec = METHOD_HEAD;
		else if (strcmp(str, "POST") == 0)
			spec = METHOD_POST;
		else
			return NULL;
	} break;

	case HDR_CONNECTION: {
		if (insensitive_string_cmp(str, "keep-alive") == 0)
			spec = CONNECTION_KEEP_ALIVE;
		else if (insensitive_string_cmp(str, "close") == 0)
			spec = CONNECTION_CLOSE;
		else
			return NULL;
	} break;

	case HDR_CONTENT_TYPE: {
		int* v = hashmap_get(_supported_media, str);
		if (!v)
			return NULL;
		spec = *v;
	} break;

	case HDR_EXPECT: {
		if (insensitive_string_cmp(str, "100-continue"))
			spec = EXPECT_CONTINUE;
		else
			return NULL;
	}
	}
	if (spec == -1)
		return NULL;
	return make_number_value(spec);
}

static struct value* parse_number(char* str, int type) {
	long n = strtol(str, NULL, 10);
	if (n == 0L)
		return NULL;
	return make_number_value(n);
}

static struct value* parse_text(char* str, int type) {
	return make_text_value(str);
}

static int validate_header(char* header) {
	int* v = hashmap_get(_supported_headers, header);
	if (!v) return HDR_NOT_SUPPORTED;
	return *v;
}

static unsigned int hash_string(const char* str, unsigned int seed) {
	char* ls = _lowered_string;
	int  len = 0;
	while (*str) {
		*ls++ = tolower(*str++);
		++len;
	}
	ls[len] = 0;
	return hashmap_murmur(ls, len, seed);
}

static int insensitive_string_cmp(const char* str1, const char* str2) {
	while (*str1 && *str2) {
		if (tolower(*str1) != tolower(*str2))
			return *str1 - *str2;
		++str1;
		++str2;
	}
	return *str1 - *str2;
}

int request_parser_end(void) {
	hashmap_free(_supported_headers);
	free(_lowered_string);
	return 0;
}

int request_parser_init(void) {
	_lowered_string = malloc(100);
	if (!_lowered_string) {
		fprintf(stderr, "[request_info_init] failed to allocate memory\n");
		return 1;
	}
	_supported_headers = hashmap_new(sizeof(char*), sizeof(int), 0, hash_string, insensitive_string_cmp, NULL, NULL);
	if (!_supported_headers) {
		free(_lowered_string);
		fprintf(stderr, "[request_info_init] hashmap_new() failed - %s.\n", hashmap_error());
		return 1;
	}
	_supported_media = hashmap_new(sizeof(char*), sizeof(int), 0, hash_string, insensitive_string_cmp, NULL, NULL); 
	if (!_supported_media) {
		free(_lowered_string);
		hashmap_free(_supported_headers);
		fprintf(stderr, "[request_info_init] hashmap_new() failed - %s.\n", hashmap_error());
		return 1;
	}

	hashmap_resize(_supported_headers, 100);
	hashmap_set(_supported_headers, &"A-IM", &(int){HDR_A_IM});
	hashmap_set(_supported_headers, &"Accept", &(int){HDR_ACCEPT});
	hashmap_set(_supported_headers, &"Accept-Charset", &(int){HDR_ACCEPT_CHARSET});
	hashmap_set(_supported_headers, &"Accept-Datetime", &(int){HDR_ACCEPT_DATETIME});
	hashmap_set(_supported_headers, &"Accept-Encoding", &(int){HDR_ACCEPT_ENCODING});
	hashmap_set(_supported_headers, &"Accept-Language", &(int){HDR_ACCEPT_LANGUAGE});
	hashmap_set(_supported_headers, &"Access-Control-Request-Method", &(int){HDR_ACCESS_CONTROL_REQUEST_METHOD});
	hashmap_set(_supported_headers, &"Access-Control-Request-Headers", &(int){HDR_ACCESS_CONTROL_REQUEST_HEADERS});
	hashmap_set(_supported_headers, &"Authorization", &(int){HDR_AUTHORIZATION});
	hashmap_set(_supported_headers, &"Cache-Control", &(int){HDR_CACHE_CONTROL});
	hashmap_set(_supported_headers, &"Connection", &(int){HDR_CONNECTION});
	hashmap_set(_supported_headers, &"Content-Encoding", &(int){HDR_CONTENT_ENCODING});
	hashmap_set(_supported_headers, &"Content-Length", &(int){HDR_CONTENT_LENGTH});
	hashmap_set(_supported_headers, &"Content-Type", &(int){HDR_CONTENT_TYPE});
	hashmap_set(_supported_headers, &"Cookie", &(int){HDR_COOKIE});
	hashmap_set(_supported_headers, &"Date", &(int){HDR_DATE});
	hashmap_set(_supported_headers, &"Expect", &(int){HDR_EXPECT});
	hashmap_set(_supported_headers, &"Forwarded", &(int){HDR_FORWARDED});
	hashmap_set(_supported_headers, &"From", &(int){HDR_FROM});
	hashmap_set(_supported_headers, &"Host", &(int){HDR_HOST});
	hashmap_set(_supported_headers, &"connection", &(int){HDR_CONNECTION});
	hashmap_set(_supported_headers, &"If-Match", &(int){HDR_IF_MATCH});
	hashmap_set(_supported_headers, &"If-Modified-Since", &(int){HDR_IF_MODIFIED_SINCE});
	hashmap_set(_supported_headers, &"If-None-Match", &(int){HDR_IF_NONE_MATCH});
	hashmap_set(_supported_headers, &"If-Range", &(int){HDR_IF_RANGE});
	hashmap_set(_supported_headers, &"If-Unmodified-Since", &(int){HDR_IF_UNMODIFIED_SINCE});
	hashmap_set(_supported_headers, &"Max-Forwards", &(int){HDR_MAX_FORWARDS});
	hashmap_set(_supported_headers, &"Origin", &(int){HDR_ORIGIN});
	hashmap_set(_supported_headers, &"Pragma", &(int){HDR_PRAGMA});
	hashmap_set(_supported_headers, &"Prefer", &(int){HDR_PREFER});
	hashmap_set(_supported_headers, &"Proxy-Authorization", &(int){HDR_PROXY_AUTHORIZATION});
	hashmap_set(_supported_headers, &"Range", &(int){HDR_RANGE});
	hashmap_set(_supported_headers, &"Referer", &(int){HDR_REFERER});
	hashmap_set(_supported_headers, &"TE", &(int){HDR_TE});
	hashmap_set(_supported_headers, &"Trailer", &(int){HDR_TRAILER});
	hashmap_set(_supported_headers, &"Transfer-Encoding", &(int){HDR_TRANSFER_ENCODING});
	hashmap_set(_supported_headers, &"User-Agent", &(int){HDR_USER_AGENT});
	hashmap_set(_supported_headers, &"Upgrade", &(int){HDR_UPGRADE});
	hashmap_set(_supported_headers, &"Via", &(int){HDR_VIA});


	hashmap_resize(_supported_headers, 200);
	hashmap_set(_supported_media, &"audio/aac", &(int){MEDIA_ACC});
	hashmap_set(_supported_media, &"application/x-abiword", &(int){MEDIA_ABW});
	hashmap_set(_supported_media, &"image/apng", &(int){MEDIA_APNG});
	hashmap_set(_supported_media, &"application/x-freearc", &(int){MEDIA_ARC});
	hashmap_set(_supported_media, &"image/avif", &(int){MEDIA_AVIF});
	hashmap_set(_supported_media, &"video/x-msvideo", &(int){MEDIA_AVI});
	hashmap_set(_supported_media, &"application/vnd.amazon.ebook", &(int){MEDIA_AZW});
	hashmap_set(_supported_media, &"application/octet-stream", &(int){MEDIA_OCTET_STREAM});
	hashmap_set(_supported_media, &"image/bmp", &(int){MEDIA_BMP});
	hashmap_set(_supported_media, &"application/x-bzip", &(int){MEDIA_BZIP});
	hashmap_set(_supported_media, &"application/x-bzip2", &(int){MEDIA_BZIP2});
	hashmap_set(_supported_media, &"application/x-cdf", &(int){MEDIA_X_CDF});
	hashmap_set(_supported_media, &"application/x-csh", &(int){MEDIA_X_CSH});
	hashmap_set(_supported_media, &"text/css", &(int){MEDIA_CSS});
	hashmap_set(_supported_media, &"text/csv", &(int){MEDIA_CSV});
	hashmap_set(_supported_media, &"application/msword", &(int){MEDIA_DOC});
	hashmap_set(_supported_media, &"application/vnd.openxmlformats-officedocument.wordprocessingml.document", &(int){MEDIA_DOCX});
	hashmap_set(_supported_media, &"application/vnd.ms-fontobject", &(int){MEDIA_EOT});
	hashmap_set(_supported_media, &"application/epub+zip", &(int){MEDIA_EPUB});
	hashmap_set(_supported_media, &"application/gzip", &(int){MEDIA_GZ});
	hashmap_set(_supported_media, &"text/html", &(int){MEDIA_HTML});
	hashmap_set(_supported_media, &"image/vnd.microsoft.icon", &(int){MEDIA_ICO});
	hashmap_set(_supported_media, &"image/jpeg", &(int){MEDIA_JPEG});
	hashmap_set(_supported_media, &"text/javascript", &(int){MEDIA_JS});
	hashmap_set(_supported_media, &"audio/midi", &(int){MEDIA_MIDI});
	hashmap_set(_supported_media, &"text/javascript", &(int){MEDIA_MJS});
	hashmap_set(_supported_media, &"audio/mpeg", &(int){MEDIA_MP3});
	hashmap_set(_supported_media, &"video/mp4", &(int){MEDIA_MP4});
	hashmap_set(_supported_media, &"video/mpeg", &(int){MEDIA_MPEG});
	hashmap_set(_supported_media, &"application/vnd.apple.installer+xml", &(int){MEDIA_MPKG});
	hashmap_set(_supported_media, &"application/vnd.oasis.opendocument.presentation", &(int){MEDIA_ODP});
	hashmap_set(_supported_media, &"application/vnd.oasis.opendocument.spreadsheet", &(int){MEDIA_ODS});
	hashmap_set(_supported_media, &"application/vnd.oasis.opendocument.text", &(int){MEDIA_ODT});
	hashmap_set(_supported_media, &"audio/ogg", &(int){MEDIA_OGA});
	hashmap_set(_supported_media, &"video/ogg", &(int){MEDIA_OGV});
	hashmap_set(_supported_media, &"application/ogg", &(int){MEDIA_OGX});
	hashmap_set(_supported_media, &"audio/opus", &(int){MEDIA_OPUS});
	hashmap_set(_supported_media, &"font/otf", &(int){MEDIA_OTF});
	hashmap_set(_supported_media, &"image/png", &(int){MEDIA_PNG});
	hashmap_set(_supported_media, &"application/pdf", &(int){MEDIA_PDF});
	hashmap_set(_supported_media, &"application/php", &(int){MEDIA_PHP});
	hashmap_set(_supported_media, &"application/vnd.ms-powerpoint", &(int){MEDIA_PPT});
	hashmap_set(_supported_media, &"application/vnd.openxmlformats-officedocument.presentationml.presentation", &(int){MEDIA_PPTX});
	hashmap_set(_supported_media, &"application/vnd.rar", &(int){MEDIA_RAR});
	hashmap_set(_supported_media, &"application/rtf", &(int){MEDIA_RTF});
	hashmap_set(_supported_media, &"application/x-sh", &(int){MEDIA_SH});
	hashmap_set(_supported_media, &"image/svg+xml", &(int){MEDIA_SVG});
	hashmap_set(_supported_media, &"application/x-tar", &(int){MEDIA_TAR});
	hashmap_set(_supported_media, &"image/tiff", &(int){MEDIA_TIFF});
	hashmap_set(_supported_media, &"application/typescript", &(int){MEDIA_TS});
	hashmap_set(_supported_media, &"font/ttf", &(int){MEDIA_TTF});
	hashmap_set(_supported_media, &"text/plain", &(int){MEDIA_TXT});
	hashmap_set(_supported_media, &"application/vnd.visio", &(int){MEDIA_VSD});
	hashmap_set(_supported_media, &"audio/wav", &(int){MEDIA_WAV});
	hashmap_set(_supported_media, &"audio/webm", &(int){MEDIA_WEBA});
	hashmap_set(_supported_media, &"video/webm", &(int){MEDIA_WEBM});
	hashmap_set(_supported_media, &"image/webp", &(int){MEDIA_WEBP});
	hashmap_set(_supported_media, &"font/woff", &(int){MEDIA_WOFF});
	hashmap_set(_supported_media, &"font/woff2", &(int){MEDIA_WOFF2});
	hashmap_set(_supported_media, &"application/xhtml+xml", &(int){MEDIA_XHTML});
	hashmap_set(_supported_media, &"application/vnd.ms-excel", &(int){MEDIA_XLS});
	hashmap_set(_supported_media, &"application/vnd.openxmlformats-officedocument.spreadsheetml.sheet", &(int){MEDIA_XLSX});
	hashmap_set(_supported_media, &"application/xml", &(int){MEDIA_XML});
	hashmap_set(_supported_media, &"application/vnd.mozilla.xul+xml", &(int){MEDIA_XUL});
	hashmap_set(_supported_media, &"application/zip", &(int){MEDIA_ZIP});
	hashmap_set(_supported_media, &"video/3gpp", &(int){MEDIA_3GP});
	hashmap_set(_supported_media, &"video/3gpp2", &(int){MEDIA_3G2});
	hashmap_set(_supported_media, &"application/x-7z-compressed", &(int){MEDIA_7Z});


	parsers[HDR_A_IM]                           = parse_list;
	parsers[HDR_ACCEPT]                         = parse_list;
	parsers[HDR_ACCEPT_CHARSET]                 = parse_list;
	parsers[HDR_ACCEPT_DATETIME]                = parse_date;
	parsers[HDR_ACCEPT_ENCODING]                = parse_list;
	parsers[HDR_ACCEPT_LANGUAGE]                = parse_list;
	parsers[HDR_ACCESS_CONTROL_REQUEST_HEADERS] = parse_list;
	parsers[HDR_ACCESS_CONTROL_REQUEST_METHOD]  = parse_specifier;
	parsers[HDR_AUTHORIZATION]                  = parse_text;
	parsers[HDR_CACHE_CONTROL]                  = parse_list;
	parsers[HDR_CONNECTION]                     = parse_specifier;
	parsers[HDR_CONTENT_ENCODING]               = parse_list;
	parsers[HDR_CONTENT_LENGTH]                 = parse_number;
	parsers[HDR_CONTENT_TYPE]                   = parse_specifier;
	parsers[HDR_COOKIE]                         = parse_list;
	parsers[HDR_DATE]                           = parse_date;
	parsers[HDR_EXPECT]                         = parse_specifier;
	parsers[HDR_FORWARDED]                      = parse_list;
	parsers[HDR_FROM]                           = parse_text;
	parsers[HDR_HOST]                           = parse_host;
	parsers[HDR_IF_MATCH]                       = parse_list;
	parsers[HDR_IF_MODIFIED_SINCE]              = parse_date;
	parsers[HDR_IF_NONE_MATCH]                  = parse_list;
	parsers[HDR_IF_RANGE]                       = parse_options;
	parsers[HDR_IF_UNMODIFIED_SINCE]            = parse_date;
	parsers[HDR_MAX_FORWARDS]                   = parse_number;
	parsers[HDR_ORIGIN]                         = parse_host;
	parsers[HDR_PRAGMA]                         = parse_text;
	parsers[HDR_PREFER]                         = parse_list;
	parsers[HDR_PROXY_AUTHORIZATION]            = parse_text;
	parsers[HDR_RANGE]                          = parse_list;
	parsers[HDR_REFERER]                        = parse_host; 
	parsers[HDR_TE]                             = parse_list;
	parsers[HDR_TRANSFER_ENCODING]              = parse_list;
	parsers[HDR_USER_AGENT]                     = parse_text;
	parsers[HDR_UPGRADE]                        = parse_list;
	parsers[HDR_VIA]                            = parse_list;
	return 0;
}

int parse_request(struct client_info* client) {
	struct request_info* req = &client->request;
	int len = 0;
	char* q = client->buffer;
	char* begin = q;
	char* end = q + client->bufflen - 1;

	while (q <= end && strstr(q, "\r\n")) {
		if (req->method == METHOD_NONE) {
			begin = q;
			q = strchr(q, ' ');
			if (!q)
				return 1;
			*q = 0;
			if (strcmp(begin, "GET") == 0)
				req->method = METHOD_GET;
			else if (strcmp(begin, "HEAD") == 0)
				req->method = METHOD_HEAD;
			else if (strcmp(begin, "POST") == 0)
				req->method = METHOD_POST;
			else
				return 1;

			++q;
			begin = q;
			if (q > end)
				return 1;
			if (*begin != '/')
				return 1;
			q = strchr(q, ' ');
			if (!q)
				return 1;
			*q = 0;
			if ((len = strlen(begin)) > RESOURCE_BUFFLEN)
				return 1;
			if (strstr(begin, ".."))
				return 1;
			memcpy(req->resource, begin, len);

			++q;
			begin = q;
			if (q > end)
				return 1;
			q = strchr(q, ' ');
			if (!q)
				return 1;
			*q = 0;
			if (strcmp(begin, "HTTP/1.0") == 0)
				req->version = HTTP_VERSION_1;
			else if (strcmp(begin, "HTTP/1.1") == 0)
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

		else if (req->finished) {
			while (q <= end && req->body_len < BODY_BUFFLEN) 
				req->body[req->body_len++] = *q++; 

			if (q <= end && req->body_len == BODY_BUFFLEN)
				return 1;
		}

		else {
			if (q <= end - 2 && memcmp(q, "\r\n", 2) == 0) {
				if (req->method == METHOD_NONE)
					return 1;
				req->finished = 1;
				return 0;
			}

			begin = q;
			q = strchr(q, ':');
			if (!q)
				return 0;
			*q = 0;
			int header_type = validate_header(begin);
			if (header_type == HDR_NOT_SUPPORTED)
				return 1;
			q++;
			while(isspace(*q)) ++q; 
			begin = q;
			if (q > end)
				return 1;
			q = strstr(q, "\r\n");
			if (!q)
				return 1;
			*q = 0;
			value_parser parser = parsers[header_type];
			struct value* parsed = parser(begin, header_type);
			if (!parsed)
				return 1;
			add_value_request_info(req, parsed, header_type);
			q += 2;
		}
	}
	client->bufflen = MAX(0, client->bufflen - (q - client->buffer));
	memcpy(client->buffer, q, client->bufflen);
	return 0;
}