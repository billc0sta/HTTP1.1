#ifndef HASHMAP_H_
#define HASHMAP_H_
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h> 
#define LOAD_FACTOR_MAX 0.6
#define LOAD_FACTOR_MIN 0.1
#define INITIAL_BUCKETS 16
#define MULTIPLY_SPACE 2

#define STATE_UNUSED 0
#define STATE_USED 1
#define STATE_DELETED 2

static char _hashmap_last_error[64] = { '\0' };

static struct hash_node
{
	void* key;
	void* val;
	char state;
};

typedef struct
{
	struct hash_node* buckets;
	void* data;
	size_t mapped;
	size_t key_size;
	size_t val_size;
	size_t space;
	size_t length;
	unsigned int (*hasher)(const void*, unsigned int seed);
	int (*cmp)(const void*, const void*);
	void (*destroy_val)(const void*);
	void (*destroy_key)(const void*);
	unsigned int seed;
} hashmap;

static hashmap* hashmap_new(size_t key_size, size_t val_size, unsigned int seed,
	unsigned int hasher(const void*, unsigned int), int cmp(const void*, const void*),
	void key_destructor(const void*), void value_destructor(const void*))
{
	int               mfail = 0;
	hashmap* out = (hashmap*)malloc(sizeof(hashmap));
	struct hash_node* buckets = (struct hash_node*)calloc(INITIAL_BUCKETS, sizeof(struct hash_node));
	void* data = malloc(key_size * INITIAL_BUCKETS + val_size * INITIAL_BUCKETS);

	if (!out || !buckets || !data)
		mfail = 1;
	if (mfail)
	{
		if (out) free(out);
		if (data) free(data);
		if (buckets) free(buckets);
		strcpy(_hashmap_last_error, "malloc failed!\n");
		return NULL;
	}
	out->mapped = 0;
	out->length = 0;
	out->buckets = buckets;
	out->data = data;
	out->key_size = key_size;
	out->val_size = val_size;
	out->space = INITIAL_BUCKETS;
	out->hasher = hasher;
	out->cmp = cmp;
	out->destroy_key = key_destructor;
	out->destroy_val = value_destructor;
	out->seed = seed;
	return out;
}

// murmur hash 
static inline unsigned int _murmur_32_scramble(unsigned int k) {
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
		h ^= _murmur_32_scramble(k);
		h = (h << 13) | (h >> 19);
		h = h * 5 + 0xe6546b64;
	}
	k = 0;
	for (size_t i = size & 3; i; i--) {
		k <<= 8;
		k |= ((char*)key)[i - 1];
	}
	h ^= _murmur_32_scramble(k);
	h ^= size;
	h ^= h >> 16;
	h *= 0x85ebca6b;
	h ^= h >> 13;
	h *= 0xc2b2ae35;
	h ^= h >> 16;
	return h;
}

static unsigned int _hash(hashmap* map, void* key)
{
	if (map->hasher) return map->hasher(key, map->seed) % map->space;
	return hashmap_murmur(key, map->key_size, map->seed) % map->space;
}

static int _compare(hashmap* map, void* key1, void* key2)
{
	if (map->cmp) return map->cmp(key1, key2);
	return memcmp(key1, key2, map->key_size);
}

static struct hash_node* _hashmap_find(hashmap* map, void* key)
{
	unsigned int i = _hash(map, key);
	struct hash_node* bucket;
	while ((bucket = &map->buckets[i])->state != STATE_UNUSED && _compare(map, key, bucket->key) != 0)
		i = (i + 1) % map->space;

	return bucket;
}

static int hashmap_set(hashmap* map, void* key, void* value);

static int hashmap_resize(hashmap* map, size_t resize_by)
{
	if (!map)
	{
		strcpy(_hashmap_last_error, "passed null pointer!\n");
		return 1;
	}
	if (resize_by == 0)
		resize_by = INITIAL_BUCKETS;

	if (resize_by == map->space)
		return 0;

	map->length = 0;
	map->mapped = 0;
	int old_space = map->space;
	map->space = resize_by;
	struct hash_node* old_buckets = map->buckets;
	void* old_data = map->data;
	struct hash_node* new_buckets = (struct hash_node*)calloc(map->space, sizeof(struct hash_node));
	void* new_data = malloc(map->key_size * map->space + map->val_size * map->space);

	int mfail = 0;
	if (!new_buckets || !new_data) mfail = 1;
	if (mfail)
	{
		if (new_buckets) free(new_buckets);
		if (new_data) free(new_data);
		strcpy(_hashmap_last_error, "malloc failed\n");
		return 1;
	}
	map->buckets = new_buckets;
	map->data = new_data;
	for (int i = 0; i < old_space; ++i) 
	{
		struct hash_node* curr = &old_buckets[i];
		if (curr->state == STATE_USED)
			hashmap_set(map, curr->key, curr->val);
		if (curr->state == STATE_DELETED)
		{
			if (map->destroy_key) map->destroy_key(curr->key);
			if (map->destroy_val) map->destroy_val(curr->val);
		}
	} 
	free(old_buckets);
	free(old_data);

	return 0;
}

static int hashmap_set(hashmap* map, void* key, void* value)
{
	if (!map || !key || !value)
	{
		strcpy(_hashmap_last_error, "passed null pointer!\n");
		return 1;
	}

	unsigned int i = _hash(map, key);
	struct hash_node* deleted_bucket = NULL;
	struct hash_node* bucket         = NULL;
	while ((bucket = &map->buckets[i])->state != STATE_UNUSED && _compare(map, key, bucket->key) != 0) 
	{
		if (bucket->state == STATE_DELETED)
			deleted_bucket = bucket;
		i = (i + 1) % map->space;
	}
	if (deleted_bucket) 
	{
		if (bucket->state == STATE_USED)
			bucket->state = STATE_DELETED;
		bucket = deleted_bucket;
	} 

	if (bucket->state == STATE_UNUSED)
	{
		bucket->key = (char*)map->data + map->key_size * map->mapped;
		bucket->val = (char*)map->data + map->key_size * map->space + map->val_size * map->mapped;
		++map->mapped;
	}
	else 
	{
		if (map->destroy_val) map->destroy_val(bucket->val);
		if (map->destroy_key) map->destroy_key(bucket->key);
	}

	memcpy(bucket->key, key, map->key_size);
	memcpy(bucket->val, value, map->val_size);
	bucket->state = STATE_USED;
	++map->length;

	if ((float)map->length / map->space >= LOAD_FACTOR_MAX)
		hashmap_resize(map, map->space * MULTIPLY_SPACE);

	return 0;
}

static void* hashmap_get(hashmap* map, void* key)
{
	if (!map || !key)
	{
		strcpy(_hashmap_last_error, "passed null pointer!\n");
		return NULL;
	}
	struct hash_node* found = _hashmap_find(map, key);
	if (found->state == STATE_USED)
		return found->val;

	strcpy(_hashmap_last_error, "no such key!\n");
	return NULL;
}

static int hashmap_remove(hashmap* map, void* key)
{
	if (!map || !key)
	{
		strcpy(_hashmap_last_error, "passed null pointer!\n");
		return 1;
	}
	struct hash_node* found = _hashmap_find(map, key);
	if (found->state != STATE_USED)
	{
		strcpy(_hashmap_last_error, "no such key!\n");
		return 1;
	}

	found->state = STATE_DELETED;
	--map->length;

	if (map->space > INITIAL_BUCKETS && (float)map->length / map->space <= LOAD_FACTOR_MIN)
		hashmap_resize(map, map->space / MULTIPLY_SPACE);

	return 0;
}

static int hashmap_clear(hashmap* map)
{
	if (!map)
	{
		strcpy(_hashmap_last_error, "passed null pointer!\n");
		return 1;
	}
	for (int i = 0; i < map->space; ++i)
	{
		struct hash_node* node = &map->buckets[i];
		if (node->state != STATE_UNUSED)
		{
			if (map->destroy_key) map->destroy_key(node->key);
			if (map->destroy_val) map->destroy_val(node->val);
			node->state = STATE_UNUSED;
		}
	}
	map->mapped = 0;
	map->length = 0;
	return 0;
}

static int hashmap_free(hashmap* map)
{
	if (!map)
	{
		strcpy(_hashmap_last_error, "passed null pointer!\n");
		return 1;
	}
	hashmap_clear(map);
	free(map->buckets);
	free(map->data);
	free(map);
	return 0;
}

static int hashmap_next(hashmap* map, size_t* iter, void** key, void** val)
{
	if (!map || !iter || !key || !val)
	{
		strcpy(_hashmap_last_error, "passed null pointer!\n");
		return 1;
	}

	while (*iter < map->space) 
	{
		struct hash_node* node = &map->buckets[*iter];
		++(*iter);
		if (node->state == STATE_USED) 
		{
			*key = node->key;
			*val = node->val;
			return 0;
		}
	} 
	return 1; 
}

static int hashmap_scan(hashmap* map, void iter_func(void*, void*))
{
	if (!map)
	{
		strcpy(_hashmap_last_error, "passed null pointer!\n");
		return 1;
	}
	if (map->length == 0)
	{
		strcpy(_hashmap_last_error, "empty map!\n");
		return 1;
	}
	size_t iter = 0; 
	void* key = NULL;
	void* val = NULL;
	while (hashmap_next(map, &iter, &key, &val))
	{
		iter_func(key, val);
	}
	return 0;
}

static const char* hashmap_error()
{ 
	return _hashmap_last_error;
}

static size_t hashmap_count(hashmap* map)
{
	if (!map)
	{
		strcpy(_hashmap_last_error, "passed null pointer!\n");
		return 1;
	}
	return map->length;
}

#endif