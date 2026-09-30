#ifndef APP_UTILITIES_PROCESS_QUIET_H
#define APP_UTILITIES_PROCESS_QUIET_H

#include <stddef.h>

/* Runs argv (no shell) with stdin, stdout and stderr on /dev/null and waits
 * for it. When `secret` is given, the child can read it from file
 * descriptor `secret_fd` (a pipe the secret is written to), so it never
 * appears in the argument list or the environment, where other users could
 * see it. Returns the exit status, or -1 when it could not be run. */
int process_run_quiet(char *const argv[], const void *secret, size_t secret_len, int secret_fd);

#endif
