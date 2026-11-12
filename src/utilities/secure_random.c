#include "utilities/secure_random.h"

#include <errno.h>
#include <stdlib.h>

#if defined(__APPLE__) || defined(__OpenBSD__) || defined(__FreeBSD__) || defined(__NetBSD__)
int secure_random_bytes(uint8_t *out, size_t len) {
    if (!out) return -1;
    arc4random_buf(out, len);
    return 0;
}
#elif defined(__linux__)
#include <sys/random.h>

int secure_random_bytes(uint8_t *out, size_t len) {
    if (!out) return -1;
    size_t got = 0;
    while (got < len) {
        ssize_t n = getrandom(out + got, len - got, 0);
        if (n < 0 && errno == EINTR) continue;
        if (n <= 0) return -1;
        got += (size_t)n;
    }
    return 0;
}
#else
#include <fcntl.h>
#include <unistd.h>

int secure_random_bytes(uint8_t *out, size_t len) {
    if (!out) return -1;
    int fd = open("/dev/urandom", O_RDONLY | O_CLOEXEC);
    if (fd < 0) return -1;
    size_t got = 0;
    while (got < len) {
        ssize_t n = read(fd, out + got, len - got);
        if (n < 0 && errno == EINTR) continue;
        if (n <= 0) break;
        got += (size_t)n;
    }
    close(fd);
    return got == len ? 0 : -1;
}
#endif
