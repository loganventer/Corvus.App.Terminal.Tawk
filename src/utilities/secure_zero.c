#if defined(__APPLE__)
#define __STDC_WANT_LIB_EXT1__ 1          /* memset_s */
#endif

#include "utilities/secure_zero.h"

#include <string.h>

void secure_zero(void *p, size_t n) {
    if (!p || n == 0) return;
#if defined(__APPLE__)
    memset_s(p, n, 0, n);
#elif defined(__GLIBC__) || defined(__OpenBSD__) || defined(__FreeBSD__) || defined(__NetBSD__) || defined(__CYGWIN__)
    explicit_bzero(p, n);
#else
    volatile unsigned char *b = p;
    while (n--) *b++ = 0;
#endif
}
