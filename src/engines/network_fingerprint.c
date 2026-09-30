#include "engines/network_fingerprint.h"

static uint64_t fnv1a(const char *s) {
    uint64_t h = 1469598103934665603ULL;
    for (; *s; s++) {
        h ^= (unsigned char)*s;
        h *= 1099511628211ULL;
    }
    return h;
}

/* Spreads the bits of one entry's hash so that summing several stays
 * sensitive to every entry (a plain sum of FNV values collides easily). */
static uint64_t mix(uint64_t h) {
    h ^= h >> 33;
    h *= 0xFF51AFD7ED558CCDULL;
    h ^= h >> 33;
    h *= 0xC4CEB9FE1A85EC53ULL;
    h ^= h >> 33;
    return h;
}

uint64_t network_fingerprint_of(const char *const *entries, int count) {
    uint64_t sum = 0;
    for (int i = 0; i < count; i++) {
        if (entries[i]) sum += mix(fnv1a(entries[i]));   /* addition: independent of order */
    }
    return count > 0 ? mix(sum ^ (uint64_t)count) : 0;
}
