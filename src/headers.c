#ifndef HASHMAP_H_
#define HASHMAP_H_
#include "headers.h"
#define LOAD_FACTOR_MAX 0.6
#define LOAD_FACTOR_MIN 0.1
#define INITIAL_BUCKETS 16
#define MULTIPLY_SPACE (1 * 2) 
#define HEADERS_SEED 0

#define STATE_UNUSED 0
#define STATE_USED 1
#define STATE_DELETED 2

headers* make_headers(void) {
  headers* ret = (headers*)malloc(sizeof(headers));
  if (!ret) {
    HTTP_LOG(HTTP_LOGERR, "[make_headers] failed to allocate memory.\n");
    return NULL;
  }
  ret->buckets = (struct bucket*)calloc(INITIAL_BUCKETS, sizeof(struct bucket));
  if (!ret->buckets) {
    free(ret);
    HTTP_LOG(HTTP_LOGERR, "[make_headers] failed to allocate memory.\n");
    return NULL;
  }
  ret->cap = INITIAL_BUCKETS;
  ret->len = 0;
  return ret;
}

static inline unsigned int murmur_scramble(unsigned int k) {
  k *= 0xcc9e2d51;
  k = (k << 15) | (k >> 17);
  k *= 0x1b873593;
  return k;
}

static unsigned int hashstring_murmur(const char* key, size_t size)
{
  unsigned int h = HEADERS_SEED;
  unsigned int k = 0;
  for (size_t i = size >> 2; i; i--) {
    k |= tolower(*key++) & 0xff;
    k |= ((tolower(*key++) & 0xff) << 8);
    k |= ((tolower(*key++) & 0xff) << 16);
    k |= ((tolower(*key++) & 0xff) << 24);
    h ^= murmur_scramble(k);
    h = (h << 13) | (h >> 19);
    h = h * 5 + 0xe6546b64;
  }
  k = 0;
  for (size_t i = size & 3; i; i--) {
    k <<= 8;
    k |= tolower(key[i - 1]);
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

static int compare(char* str1, char* str2) {
  while (*str1 && *str2) {
    if (tolower(*str1) != tolower(*str2))
      return *str1 - *str2;
    ++str1;
    ++str2;
  }
  return *str1 - *str2;
}

static struct bucket* headers_find(headers* map, char* key)
{
  unsigned int i = hashstring_murmur(key, strlen(key)) & (map->cap - 1);
  struct bucket* bucket;
  while ((bucket = &map->buckets[i])->state != STATE_UNUSED && compare(key, bucket->key) != 0)
    i = (i + 1) & (map->cap - 1);

  return bucket;
}

int set_header(headers* map, char* key, char* val) {
  if (!map || !key || !val) {
    HTTP_LOG(HTTP_LOGERR, "[set_header] passed NULL pointers for mandatory parameters.\n");
    return 1;
  }
  unsigned int i = hashstring_murmur(key, strlen(key)) & (map->cap - 1);
  struct bucket* deleted_bucket = NULL;
  struct bucket* bucket = NULL;

  while ((bucket = &map->buckets[i])->state != STATE_UNUSED && compare(key, bucket->key) != 0) {
    if (bucket->state == STATE_DELETED)
      deleted_bucket = bucket;
    i = (i + 1) & (map->cap - 1);
  }
  if (deleted_bucket) {
    if (bucket->state == STATE_USED)
      bucket->state = STATE_DELETED;
    bucket = deleted_bucket;
  }

  int keylen = (int)strlen(key);
  int vallen = (int)strlen(val);
  struct value* v = (struct value*)malloc(sizeof(struct value) + vallen + 1);
  if (!v) {
    HTTP_LOG(HTTP_LOGERR, "[set_header] failed to allocate memory.\n");
    return 1;
  }
  v->next = NULL;
  v->v = (char*)(v + 1);
  v->v[vallen] = 0;
  v->len = vallen;
  memcpy(v->v, val, vallen);
  if (bucket->state == STATE_UNUSED) {
    bucket->key = (char*)malloc(keylen + 1);
    if (!bucket->key) {
      free(v);
      HTTP_LOG(HTTP_LOGERR, "[set_header] failed to allocate memory.\n");
      return 1;
    }
  }
  else	{
    if (strlen(bucket->key) < keylen) {
      free(bucket->key);
      bucket->key = (char*)malloc(keylen + 1);
      if (!bucket->key) {
        free(v);
        HTTP_LOG(HTTP_LOGERR, "[set_header] failed to allocate memory.\n");
        return 1;
      }
    }
    struct value* prev = NULL;
    for (struct value* val = bucket->val; val; val = val->next) {
      prev = val;
      free(prev); // also deallocates the string
    }
  }
  bucket->key[keylen] = 0; 
  memcpy(bucket->key, key, keylen); 
  bucket->state = STATE_USED;
  if (!bucket->val)
    bucket->val = v;
  else {
    v->next = bucket->val;
    bucket->val = v;
  }
  ++map->len;

  if ((float)map->len / map->cap >= LOAD_FACTOR_MAX) {
    if (resize_headers(map, map->cap * MULTIPLY_SPACE)) {
      HTTP_LOG(HTTP_LOGERR, "[set_header] hashmap_resize failed.\n");
      return 1;
    }
  }
  return 0; 
}

static int resize_headers(headers* map, size_t resize_by)
{
  if (!map) {
    HTTP_LOG(HTTP_LOGERR, "[resize_headers] passed NULL pointers for mandatory parameters.\n");
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
    HTTP_LOG(HTTP_LOGERR, "[resize_headers] failed to allocate memory.\n");
    return 1;
  }

  map->buckets = new_buckets;
  for (int i = 0; i < old_cap; ++i) {
    struct bucket* curr = &old_buckets[i];
    if (curr->state == STATE_USED) {
      struct bucket* bucket = headers_find(map, curr->key); // safe
      *bucket = *curr; 
    }

    else if (curr->state == STATE_DELETED) {
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

struct value* get_header(headers* map, char* key)
{
  if (!map || key) {
    HTTP_LOG(HTTP_LOGERR, "[get_header] passed NULL pointers for mandatory parameters.\n");
    return NULL;
  } 
  struct bucket* found = headers_find(map, key); 
  if (found->state == STATE_USED)
    return found->val;

  return NULL;
}

int remove_header(headers* map, char* key)
{
  if (!map || key) {
    HTTP_LOG(HTTP_LOGERR, "[remove_header] passed NULL pointers for mandatory parameters.\n");
    return 1;
  }

  struct bucket* found = headers_find(map, key);
  if (found->state != STATE_USED)
    return 1;

  found->state = STATE_DELETED;
  --map->len;

  if (map->cap > INITIAL_BUCKETS && (float)map->len / map->cap <= LOAD_FACTOR_MIN) {
    if (headers_resize(map, map->cap / MULTIPLY_SPACE)) {
      HTTP_LOG(HTTP_LOGERR, "[remove_header] hashmap_resize failed.\n");
      return 1;
    }
  }

  return 0;
}

int next_header(headers* map, size_t* iter, char** key, struct value** val)
{
  if (!map || !iter || !key || !val) {
    HTTP_LOG(HTTP_LOGERR, "[next_header] passed NULL pointers for mandatory parameters.\n");
    return 1;
  }

  while (*iter < map->cap) {
    struct bucket* bucket = &map->buckets[*iter];
    ++(*iter);
    if (bucket->state == STATE_USED)
      {
        *key = bucket->key;
        *val = bucket->val;
        return 0;
      }
  }
  return 1;
}

int reset_headers(headers* map)
{
  if (!map) {
    HTTP_LOG(HTTP_LOGERR, "[reset_headers] passed NULL pointers for mandatory parameters.\n");
    return 1;
  }
  for (int i = 0; i < map->cap; ++i) {
    struct bucket* bucket = &map->buckets[i];
    if (bucket->state != STATE_UNUSED) {
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

int free_headers(headers* map) {
  if (!map) {
    HTTP_LOG(HTTP_LOGERR, "[free_headers] passed NULL pointers for mandatory parameters.\n");
    return 1;
  }
  reset_headers(map);
  free(map->buckets);
  free(map);
  return 0;
}
#endif
