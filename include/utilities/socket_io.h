#ifndef APP_UTILITIES_SOCKET_IO_H
#define APP_UTILITIES_SOCKET_IO_H

#include <stddef.h>
#include <sys/types.h>

/* Local sockets that behave the same on Linux and macOS: closed on exec, and
 * a peer that hangs up gives EPIPE rather than a SIGPIPE that ends tawk.
 * Linux has SOCK_CLOEXEC and MSG_NOSIGNAL; macOS has neither, so the flag
 * is set afterwards and SO_NOSIGPIPE is set on each socket. */

/* A new AF_UNIX stream socket, or -1. */
int     socket_unix_stream(void);
/* For sockets from accept(): no SIGPIPE on macOS (a no-op elsewhere). */
void    socket_no_sigpipe(int fd);
/* send() without SIGPIPE. */
ssize_t socket_send(int fd, const void *data, size_t length);

#endif
