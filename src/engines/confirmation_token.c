#include "engines/confirmation_token.h"

#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>

int confirmation_token_generate(char *out, size_t size) {
    unsigned char bytes[16];
    if (size < sizeof(bytes) * 2 + 1) return -1;
    int fd = open("/dev/urandom", O_RDONLY | O_CLOEXEC);
    if (fd < 0) return -1;
    ssize_t n = read(fd, bytes, sizeof(bytes));
    close(fd);
    if (n != (ssize_t)sizeof(bytes)) return -1;
    for (size_t i = 0; i < sizeof(bytes); i++) snprintf(out + i * 2, size - i * 2, "%02x", bytes[i]);
    return 0;
}
