#include "value.h"
#include "includes.h"

struct value* make_directive_value(int directive) {
	struct value* v = malloc(sizeof(struct value));
	if (!v) {
		fprintf(stderr, "[make_pair_value] failed to allocate memory.\n");
		return NULL;
	}
	v->type = HDRV_DIRECTIVE;
	v->next = NULL;
	v->v.directive = directive;
	return v;
}

struct value* make_pair_value(char* name, struct value* value) {
	if (!name || !value) {
		fprintf(stderr, "[make_pair_value] passed NULL pointers for mandatory parameters.\n");
		return NULL;
	}
	if (value->next != NULL) {
		fprintf(stderr, "[make_pair_value] a pair cannot contain more than one value.\n");
		return NULL;
	} 
	struct value* v = malloc(sizeof(struct value));
	if (!v) {
		fprintf(stderr, "[make_pair_value] failed to allocate memory.\n");
		return NULL;
	}
	v->type = HDRV_PAIR;
	v->next = NULL;
	v->v.pair.name = malloc(strlen(name) + 10);
	if (!v->v.pair.name) {
		free(v);
		fprintf(stderr, "[make_pair_value] failed to allocate memory.\n");
		return NULL;
	}
	strcpy(v->v.pair.name, name);
	v->v.pair.value = value;
	return v;
}

struct value* make_host_value(char* host, uint16_t port) {
	if (!host) {
		fprintf(stderr, "[make_host_value] passed NULL pointers for mandatory parameters.\n");
		return NULL;
	}
	struct host_info h;
	int len = strlen(host);
	h.hostname = malloc(len + 10);
	if (!h.hostname) {
		fprintf(stderr, "[make_host_value] failed to allocate memory.\n");
		return NULL;
	}
	h.hostname[len] = 0;
	strcpy(h.hostname, host); 
	h.scheme = NULL;
	h.port   = port;
	struct value* v = malloc(sizeof(struct value));
	if (!v) {
		free(h.hostname);
		fprintf(stderr, "[make_host_value] failed to allocate memory.\n");
		return NULL;
	}
	v->type   = HDRV_HOST;
	v->v.host = h;
	v->next   = NULL;
	return v;
} 

struct value* make_date_value(struct vdate* date) {
	if (!date) {
		fprintf(stderr, "[make_date_value] passed NULL pointers for mandatory parameters.\n");
		return NULL;
	}
	struct value* v = malloc(sizeof(struct value));
	if (!v) {
		fprintf(stderr, "[make_date_value] failed to allocate memory.\n");
		return NULL;
	}
	v->type   = HDRV_DATE;
	v->v.date = *date;
	v->next   = NULL;
	return v;
}

struct value* make_number_value(long double n) {
	struct value* v = malloc(sizeof(struct value));
	if (!v) {
		fprintf(stderr, "[make_text_value] failed to allocate memory.\n");
		return NULL;
	}
	v->type     = HDRV_NUMBER;
	v->v.number = n;
	v->next     = NULL;
	return v;
} 

struct value* make_text_value(char* str) {
	if (!str) {
		fprintf(stderr, "[free_value] passed NULL pointers for mandatory parameters.\n");
		return NULL;
	}

	struct value* v = malloc(sizeof(struct value));
	if (!v) {
		fprintf(stderr, "[make_text_value] failed to allocate memory.\n");
		return NULL;
	}
	int len = strlen(str);
	char* text = malloc(len + 10);
	if (!text) {
		free(v);
		fprintf(stderr, "[make_text_value] failed to allocate memory.\n");
		return NULL;
	}

	text[len] = 0;
	strcpy(text, text);
	v->type   = HDRV_TEXT;
	v->v.text = text;
	v->next   = NULL;
	return v;
}

int free_values(struct value* val) {
	if (!val) {
		fprintf(stderr, "[free_value] passed NULL pointers for mandatory parameters.\n");
		return 1;
	}

	struct value* prev = NULL;
	while (val) {
		prev = val; 
		val = val->next; 
		if (prev->type == HDRV_TEXT) 
			free(prev->v.text); 
		if (prev->type == HDRV_HOST) { 
			if (prev->v.host.scheme) 
				free(prev->v.host.scheme);  
			free(prev->v.host.hostname); 
		}
		free(prev);
	}
	return 0;
}