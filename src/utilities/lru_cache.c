#include "utilities/lru_cache.h"

#include <stdlib.h>
#include <string.h>

/* Small capacities (tens to a few hundred entries) make a linear scan over a
 * recency-ordered array simpler and faster than a hash map plus list. */
typedef struct LruEntry {
    char *key;
    void *value;
} LruEntry;

struct LruCache {
    LruEntry    *entries;   /* index 0 is the most recently used */
    int          count;
    int          capacity;
    LruValueFree free_value;
};

LruCache *lru_cache_create(int capacity, LruValueFree free_value) {
    if (capacity <= 0) return NULL;
    LruCache *c = calloc(1, sizeof(*c));
    if (!c) return NULL;
    c->entries = calloc((size_t)capacity, sizeof(LruEntry));
    if (!c->entries) { free(c); return NULL; }
    c->capacity = capacity;
    c->free_value = free_value;
    return c;
}

static void release(LruCache *c, LruEntry *e) {
    free(e->key);
    if (c->free_value && e->value) c->free_value(e->value);
    e->key = NULL;
    e->value = NULL;
}

static int find(const LruCache *c, const char *key) {
    for (int i = 0; i < c->count; i++) {
        if (strcmp(c->entries[i].key, key) == 0) return i;
    }
    return -1;
}

static void promote(LruCache *c, int index) {
    LruEntry hit = c->entries[index];
    memmove(&c->entries[1], &c->entries[0], (size_t)index * sizeof(LruEntry));
    c->entries[0] = hit;
}

void *lru_cache_get(LruCache *c, const char *key) {
    if (!c || !key) return NULL;
    int i = find(c, key);
    if (i < 0) return NULL;
    promote(c, i);
    return c->entries[0].value;
}

void lru_cache_put(LruCache *c, const char *key, void *value) {
    if (!c || !key) return;
    int i = find(c, key);
    if (i >= 0) {
        if (c->free_value && c->entries[i].value && c->entries[i].value != value) c->free_value(c->entries[i].value);
        c->entries[i].value = value;
        promote(c, i);
        return;
    }
    char *copy = strdup(key);
    if (!copy) {
        if (c->free_value && value) c->free_value(value);
        return;
    }
    if (c->count == c->capacity) {
        release(c, &c->entries[c->count - 1]);
        c->count--;
    }
    memmove(&c->entries[1], &c->entries[0], (size_t)c->count * sizeof(LruEntry));
    c->entries[0] = (LruEntry){ copy, value };
    c->count++;
}

void lru_cache_remove(LruCache *c, const char *key) {
    if (!c || !key) return;
    int i = find(c, key);
    if (i < 0) return;
    release(c, &c->entries[i]);
    memmove(&c->entries[i], &c->entries[i + 1], (size_t)(c->count - i - 1) * sizeof(LruEntry));
    c->count--;
}

void lru_cache_clear(LruCache *c) {
    if (!c) return;
    for (int i = 0; i < c->count; i++) release(c, &c->entries[i]);
    c->count = 0;
}

void lru_cache_destroy(LruCache *c) {
    if (!c) return;
    lru_cache_clear(c);
    free(c->entries);
    free(c);
}
