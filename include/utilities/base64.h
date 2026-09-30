#ifndef APP_UTILITIES_BASE64_H
#define APP_UTILITIES_BASE64_H

#include <stddef.h>

/* Decodes standard base64 (padding optional, whitespace ignored). Returns a
 * malloc'd buffer and its length, or NULL for invalid input. */
unsigned char *base64_decode(const char *text, size_t max_out, int *out_len);

/* Encodes bytes as standard base64. Returns a malloc'd string. */
char *base64_encode(const unsigned char *data, size_t len);

#endif
