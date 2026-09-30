#ifndef APP_UTILITIES_LRU_CACHE_H
#define APP_UTILITIES_LRU_CACHE_H

#include <stddef.h>

/* Frees a cached value. */
typedef void (*LruValueFree)(void *value);

/* Fixed-capacity, string-keyed, least-recently-used cache. Not thread safe;
 * owned by one component on the UI thread. */
typedef struct LruCache LruCache;

LruCache *lru_cache_create(int capacity, LruValueFree free_value);
/* Returns the value and marks it most recently used, or NULL. */
void     *lru_cache_get(LruCache *cache, const char *key);
/* Stores value (taking ownership), evicting the least recently used entry when full. */
void      lru_cache_put(LruCache *cache, const char *key, void *value);
void      lru_cache_remove(LruCache *cache, const char *key);
void      lru_cache_clear(LruCache *cache);
void      lru_cache_destroy(LruCache *cache);

#endif
