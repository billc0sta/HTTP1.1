#ifndef INCLUDES_H_
#define INCLUDES_H_

#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include "hashmap.h"
#define SELECT_SEC 5
#define SELECT_USEC 0
#define HTTP_VERSION_1 0
#define HTTP_VERSION_1_1 1
#define HTTP_VERSION_NONE 2
#define MIN(a, b) ((a < b) ? (a) : (b))
#define MAX(a, b) ((a > b) ? (a) : (b))

typedef uint32_t ipv4_t;

#endif