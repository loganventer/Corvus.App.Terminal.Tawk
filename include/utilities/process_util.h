#ifndef APP_UTILITIES_PROCESS_UTIL_H
#define APP_UTILITIES_PROCESS_UTIL_H

#include <stddef.h>

#define PROCESS_MAX_ARGS 32

/* Splits a command line into argv, honouring single and double quotes.
 * No expansion of any kind is performed. `storage` backs the strings.
 * Returns argc, or -1 when the command is empty or too long. */
int process_split_args(const char *command, char **argv, int max_args, char *storage, size_t storage_size);
/* Starts argv detached (new session, stdio on /dev/null) and does not wait.
 * The intermediate child is reaped so no zombies remain. */
int process_spawn_detached(char *const argv[]);
/* True when `name` is an executable in $PATH (or an executable path). */
int process_on_path(const char *name);

#endif
