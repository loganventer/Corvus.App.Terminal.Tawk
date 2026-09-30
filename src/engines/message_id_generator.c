#include "engines/message_id_generator.h"

#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>

int message_id_generate(char *out, size_t size) {
    unsigned char bytes[9];
    int fd = open("/dev/urandom", O_RDONLY | O_CLOEXEC);
    if (fd < 0) return -1;
    ssize_t n = read(fd, bytes, sizeof(bytes));
    close(fd);
    if (n != (ssize_t)sizeof(bytes) || size < 23) return -1;
    int used = snprintf(out, size, "3EB0");
    for (size_t i = 0; i < sizeof(bytes); i++) {
        used += snprintf(out + used, size - (size_t)used, "%02X", bytes[i]);
    }
    return 0;
}
