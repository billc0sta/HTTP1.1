#ifndef VALUE_H_
#define VALUE_H_
#include "includes.h"
#define DOMAIN_MAXLEN 254
#define DOMAIN_MINLEN 3

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

enum {
	HDRV_TEXT,
	HDRV_NUMBER,
	HDRV_DIRECTIVE,
	HDRV_PARAM,
	HDRV_DATE,
	HDRV_PAIR,
	HDRV_HOST,
	HDRV_LIST
};

enum {
	CHARSET_UTF_8,
	CHARSET_ISO_8859_1,
	CHARSET_ISO_8859_2,
	CHARSET_WINDOWS_1252,
	CHARSET_SHIFT_JIS,
	CHARSET_EUC_JP,
	CHARSET_GB2312,
	CHARSET_BIG5,
	CHARSET_ISO_8859_9,
	CHARSET_ISO_2022_JP
};

enum {
	ENCODING_GZIP,
	ENCODING_COMPRESS,
	ENCODING_DEFLATE,
	ENCODING_BR,
	ENCODING_ZSTD
};

enum {
	LANG_EN,
	LANG_EN_US,
	LANG_EN_GB,
	LANG_FR,
	LANG_FR_FR,
	LANG_DE,
	LANG_DE_DE,
	LANG_ES,
	LANG_ES_ES,
	LANG_IT,
	LANG_IT_IT,
	LANG_PT,
	LANG_PT_BR,
	LANG_ZH,
	LANG_ZH_CN,
	LANG_ZH_TW,
	LANG_JA,
	LANG_JA_JP,
	LANG_KO,
	LANG_KO_KR,
	LANG_RU,
	LANG_RU_RU,
	LANG_AR,
	LANG_AR_SA,
	LANG_NL,
	LANG_NL_NL
};

struct host_info {
	char* scheme;
	char* hostname;
	uint16_t port;
};

struct vpair {
	char* name;
	struct value* value;
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

struct value {
	int type;
	union {
		char*            text;
		long double      number;
		int              directive;
		struct vpair     pair;
		struct vdate     date;
		struct host_info host;
		struct value*    list;
	} v;
	struct value* next;
};

int free_values(struct value*);
struct value* make_text_value(char*);
struct value* make_number_value(long double);
struct value* make_date_value(struct vdate*);
struct value* make_host_value(char*, uint16_t port);
struct value* make_pair_value(char*, struct value*);
struct value* make_directive_value(int);
#endif