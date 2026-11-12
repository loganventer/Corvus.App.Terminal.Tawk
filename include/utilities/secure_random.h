#ifndef APP_UTILITIES_SECURE_RANDOM_H
#define APP_UTILITIES_SECURE_RANDOM_H

#include <stddef.h>
#include <stdint.h>

/* Fills `out` with bytes from the system's secure random source, for salts
 * and keys: arc4random_buf on macOS and the BSDs, getrandom on Linux,
 * /dev/urandom elsewhere. Returns 0, or -1 when the source cannot be read. */
int secure_random_bytes(uint8_t *out, size_t len);

#endif
