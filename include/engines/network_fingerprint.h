#ifndef APP_ENGINES_NETWORK_FINGERPRINT_H
#define APP_ENGINES_NETWORK_FINGERPRINT_H

#include <stdint.h>

/* One number that changes whenever the set of network addresses changes.
 * Each entry names one interface address ("eth0 10.0.0.2"); the order of
 * the entries does not matter. No entries give 0. */
uint64_t network_fingerprint_of(const char *const *entries, int count);

#endif
