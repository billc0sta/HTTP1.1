#ifndef VALUE_H_
#define VALUE_H_
#include "includes.h"

struct host_info {
	ipv4_t ip;
	uint16_t port;
};

struct vfield {
	char* name;
	struct value* value;
};

enum {
	DAY_SAT,
	DAY_SUN,
	DAY_MON,
	DAY_TUE,
	DAY_WED,
	DAY_THU,
	DAY_FRI
};

enum {
	MONTH_JAN,
	MONTH_FEB,
	MONTH_MAR,
	MONTH_APR,
	MONTH_MAY,
	MONTH_JUN,
	MONTH_JUL,
	MONTH_AUG,
	MONTH_SEP,
	MONTH_OCT,
	MONTH_NOV,
	MONTH_DEC
};

struct vdate {
	unsigned char day;
	unsigned char month_day;
	unsigned char month;
	unsigned short year;
	unsigned char hour;
	unsigned char minute;
	unsigned char second;
};

enum {
	HDRV_TEXT,
	HDRV_NUMBER,
	HDRV_SPECIFIER,
	HDRV_DATE,
	HDRV_FIELD,
	HDRV_HOST,
	HDRV_LIST
};

struct value {
	int type;
	union {
		char*  text;
		long   number;
		int    specifier;
		struct vdate date;
		struct vfield field;
		struct host_info host;
		struct value* list;
	} v;
	struct value* next;
	char used;
};

int free_value(struct value*);
struct value* make_text_value(char*);
struct value* make_number_value(long);
struct value* make_date_value(struct vdate*);
#endif