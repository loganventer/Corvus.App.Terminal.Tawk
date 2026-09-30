#ifndef APP_UTILITIES_SECURE_ZERO_H
#define APP_UTILITIES_SECURE_ZERO_H

#include <stddef.h>

/* Overwrites memory with zeros in a way the compiler cannot drop, for
 * passphrases and other secrets: memset_s on macOS, explicit_bzero on Linux
 * and the BSDs, a volatile loop elsewhere. */
void secure_zero(void *p, size_t n);

#endif
