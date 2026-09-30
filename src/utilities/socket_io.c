#include "utilities/socket_io.h"

#include <fcntl.h>
#include <sys/socket.h>
#include <unistd.h>

#ifdef MSG_NOSIGNAL
#define SEND_FLAGS MSG_NOSIGNAL
#else
#define SEND_FLAGS 0          /* macOS: SO_NOSIGPIPE is set on the socket instead */
#endif

void socket_no_sigpipe(int fd) {
#ifdef SO_NOSIGPIPE
    int one = 1;
    setsockopt(fd, SOL_SOCKET, SO_NOSIGPIPE, &one, sizeof(one));
#else
    (void)fd;
#endif
}

int socket_unix_stream(void) {
#ifdef SOCK_CLOEXEC
    int fd = socket(AF_UNIX, SOCK_STREAM | SOCK_CLOEXEC, 0);
#else
    int fd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (fd >= 0) fcntl(fd, F_SETFD, FD_CLOEXEC);
#endif
    if (fd >= 0) socket_no_sigpipe(fd);
    return fd;
}

ssize_t socket_send(int fd, const void *data, size_t length) {
    return send(fd, data, length, SEND_FLAGS);
}
