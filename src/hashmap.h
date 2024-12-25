#ifndef HASHMAP_H_
#define HASHMAP_H_
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h> 
#define LOAD_FACTOR_MAX 0.6
#define LOAD_FACTOR_MIN 0.1
#define INITIAL_BUCKETS 16
#define INITIAL_DATALEN 1000
#define MULTIPLY_SPACE 2

#define STATE_UNUSED 0
#define STATE_USED 1
#define STATE_DELETED 2

struct value {
	char* v;
	int len;
	struct value* next;
};

struct bucket {
	char state;
	char* key;
	struct value* val; 
};

typedef int (*strcmp_func)(char*, char*);
typedef int (*strhash_func)(char*, unsigned int);

typedef struct {
	size_t cap;
	size_t len;
	struct bucket* buckets;
	strcmp_func cmp;
	strhash_func hash;
	unsigned int seed;
} hashmap;

static hashmap* hashmap_new(strcmp_func cmp, strhash_func hash, unsigned seed) {
	hashmap* ret = malloc(sizeof(hashmap));
	if (!ret) {
		fprintf(stderr, "[hashmap_new] failed to allocate memory.\n");
		return NULL;
	}
	ret->buckets = calloc(INITIAL_BUCKETS, sizeof(struct bucket));
	if (!ret->buckets) {
		free(ret);
		fprintf(stderr, "[hashmap_new] failed to allocate memory.\n");
		return NULL;
	}
	ret->cap = INITIAL_BUCKETS;
	ret->len = 0;
	ret->cmp = cmp;
	ret->seed = seed;
	ret->hash = hash;
	return ret;
}

static inline unsigned int murmur_scramble(unsigned int k) {
	k *= 0xcc9e2d51;
	k = (k << 15) | (k >> 17);
	k *= 0x1b873593;
	return k;
}

static unsigned int hashmap_murmur(const void* key, size_t size, unsigned int seed)
{
	unsigned int h = seed;
	unsigned int k;
	for (size_t i = size >> 2; i; i--) {
		memcpy(&k, key, sizeof(unsigned int));
		key = (char*)key + sizeof(unsigned int);
		h ^= murmur_scramble(k);
		h = (h << 13) | (h >> 19);
		h = h * 5 + 0xe6546b64;
	}
	k = 0;
	for (size_t i = size & 3; i; i--) {
		k <<= 8;
		k |= ((char*)key)[i - 1];
	}
	h ^= murmur_scramble(k);
	h ^= size;
	h ^= h >> 16;
	h *= 0x85ebca6b;
	h ^= h >> 13;
	h *= 0xc2b2ae35;
	h ^= h >> 16;
	return h;
}


static unsigned int hash(hashmap* map, char* key) {
	if (map->hash) return map->hash(key, map->seed);
	return hashmap_murmur(key, strlen(key), map->seed);
}

static int compare(hashmap* map, char* str1, char* str2) {
	if (map->cmp) return map->cmp(str1, str2);
	return strcmp(str1, str2); 
}

static struct bucket* hashmap_find(hashmap* map, void* key)
{
	unsigned int i = hash(map, key);
	struct bucket* bucket;
	while ((bucket = &map->buckets[i])->state != STATE_UNUSED && compare(map, key, bucket->key) != 0)
		i = (i + 1) & (map->cap - 1);

	return bucket;
}

static int hashmap_resize(hashmap* map, size_t resize_by);
static int hashmap_set(hashmap* map, char* key, char* val) {
	if (!map || !key || !val)
	{
		fprintf(stderr, "[hashmap_set] passed NULL pointers for mandatory parameters.\n");
		return 1;
	}
	unsigned int i = hash(map, key);
	struct bucket* deleted_bucket = NULL;
	struct bucket* bucket = NULL;
	while ((bucket = &map->buckets[i])->state != STATE_UNUSED && compare(map, key, bucket->key) != 0)
	{
		if (bucket->state == STATE_DELETED)
			deleted_bucket = bucket;
		i = (i + 1) & (map->cap - 1);
	}
	if (deleted_bucket)
	{
		if (bucket->state == STATE_USED)
			bucket->state = STATE_DELETED;
		bucket = deleted_bucket;
	}

	int keylen = (int)strlen(key);
	int vallen = (int)strlen(val);
	struct value* v = malloc(sizeof(struct value) + vallen + 1);
	if (!v) {
		fprintf(stderr, "[hashmap_set] failed to allocate memory.\n");
		return 1;
	}
	v->next = NULL;
	v->v[vallen] = 0;
	v->len = vallen;
	memcpy(bucket->val->v, val, vallen);
	if (bucket->state == STATE_UNUSED)
	{
		bucket->key = malloc(keylen + 1);
		if (!bucket->key) {
			free(v);
			fprintf(stderr, "[hashmap_set] failed to allocate memory.\n");
			return 1;
		}
		bucket->key[keylen] = 0;
		memcpy(bucket->key, key, keylen);
	}
	else
	{
		if (strlen(bucket->key) < keylen)
			free(bucket->key);
		else {
			bucket->key[keylen] = 0;
			memcpy(bucket->key, key, keylen);
		}
		struct value* prev = NULL;
		for (struct value* val = bucket->val; val; val = val->next) {
			prev = val;
			free(prev); // also deallocates the string
		}
	}
	if (!bucket->val)
		bucket->val = v;
	else {
		v->next = bucket->val;
		bucket->val = v;
	}
	bucket->state = STATE_USED;
	++map->len;

	if ((float)map->len / map->cap >= LOAD_FACTOR_MAX) {
		if (hashmap_resize(map, map->cap * MULTIPLY_SPACE)) {
			fprintf(stderr, "[hashmap_set] hashmap_resize failed.\n");
			return 1;
		}
	}
	return 0; 
}

static int hashmap_resize(hashmap* map, size_t resize_by)
{
	if (!map)
	{
		fprintf(stderr, "[hahsmap_resize] passed NULL pointers for mandatory parameters.\n");
		return 1;
	}
	if (resize_by == 0)
		resize_by = INITIAL_BUCKETS;

	if (resize_by == map->cap)
		return 0;

	map->len = 0;
	int old_cap = (int)map->cap;
	map->cap = resize_by;
	struct bucket* old_buckets = map->buckets;
	struct bucket* new_buckets = (struct bucket*)calloc(map->cap, sizeof(struct bucket));
	if (!new_buckets) {
		fprintf(stderr, "[hashmap_resize] failed to allocate memory.\n");
		return 1;
	}

	map->buckets = new_buckets;
	for (int i = 0; i < old_cap; ++i)
	{
		struct bucket* curr = &old_buckets[i];
		if (curr->state == STATE_USED) {
			hashmap_find(map, curr->key)->val = curr->val; // safe
			free(curr->key); 
		}

		else if (curr->state == STATE_DELETED)
		{
			free(curr->key);
			struct value* prev = NULL;
			for (struct value* val = curr->val; val; val = val->next) {
				prev = val;
				free(prev);
			}
		}
	}
	free(old_buckets);

	return 0;
}

static struct value* hashmap_get(hashmap* map, void* key)
{
	if (!map || key)
	{
		fprintf(stderr, "[hahsmap_get] passed NULL pointers for mandatory parameters.\n");
		return NULL;
	} 
	struct bucket* found = hashmap_find(map, key); 
	if (found->state == STATE_USED)
		return found->val;

	return NULL;
}

static int hashmap_remove(hashmap* map, void* key)
{
	if (!map || key)
	{
		fprintf(stderr, "[hashmap_remove] passed NULL pointers for mandatory parameters.\n");
		return 1;
	}

	struct bucket* found = hashmap_find(map, key);
	if (found->state != STATE_USED)
		return 1;

	found->state = STATE_DELETED;
	--map->len;

	if (map->cap > INITIAL_BUCKETS && (float)map->len / map->cap <= LOAD_FACTOR_MIN) {
		if (hashmap_resize(map, map->cap / MULTIPLY_SPACE)) {
			fprintf(stderr, "[hashmap_remove] hashmap_resize failed.\n");
			return 1;
		}
	}

	return 0;
}

static int hashmap_clear(hashmap* map)
{
	if (!map)
	{
		fprintf(stderr, "[hashmap_clear] passed NULL pointers for mandatory parameters.\n");
		return 1;
	}
	for (int i = 0; i < map->cap; ++i)
	{
		struct bucket* bucket = &map->buckets[i];
		if (bucket->state != STATE_UNUSED)
		{
			free(bucket->key);
			struct value* prev = NULL;
			for (struct value* val = bucket->val; val; val = val->next) {
				prev = val;
				free(prev);
			}
			bucket->state = STATE_UNUSED;
		}
	}
	map->len = 0;
	return 0;
}
static int hashmap_free(hashmap* map)
{
	if (!map)
	{
		fprintf(stderr, "[hashmap_free] passed NULL pointers for mandatory parameters.\n");
		return 1;
	}
	hashmap_clear(map);
	free(map->buckets);
	free(map);
	return 0;
}
#endif