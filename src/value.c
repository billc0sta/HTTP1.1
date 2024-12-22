#include "value.h"
#include "includes.h"

struct value* make_date_value(struct vdate* date) {
	if (!date) {
		fprintf(stderr, "[make_date_value] passed NULL pointers for mandatory parameters.\n");
		return NULL;
	}
	struct value* v = malloc(sizeof(struct value));
	if (!v) {
		fprintf(stderr, "[make_text_value] failed to allocate memory.\n");
		return NULL;
	}
	v->type   = HDRV_DATE;
	v->v.date = *date;
	v->next   = NULL;
	return v;
}

struct value* make_number_value(long n) {
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

int free_value(struct value* val) {
	if (!val) {
		fprintf(stderr, "[free_value] passed NULL pointers for mandatory parameters.\n");
		return 1;
	}
	if (val->type == HDRV_TEXT)
		free(val->v.text);
	free(val);
	return 0;
}