#ifndef APP_ENGINES_BYTE_SIZE_TEXT_H
#define APP_ENGINES_BYTE_SIZE_TEXT_H

#include <stddef.h>

/* A size in bytes as a person reads it: "812 B", "34 KB", "1.1 MB", "2.4 GB". */
void byte_size_text(long long bytes, char *out, size_t size);

#endif
