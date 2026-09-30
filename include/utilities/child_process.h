#ifndef APP_UTILITIES_CHILD_PROCESS_H
#define APP_UTILITIES_CHILD_PROCESS_H

#include <sys/types.h>

/* A tracked child process (audio player, recorder) that the owner stops and reaps. */
typedef struct ChildProcess {
    pid_t pid;
} ChildProcess;

/* Starts argv in its own process group with stdio on /dev/null. */
int  child_process_start(ChildProcess *child, char *const argv[]);
/* Same, wiring stdin/stdout to the given fds (-1 means /dev/null). The fds
 * are closed in the parent's copy by the caller, not here. */
int  child_process_start_io(ChildProcess *child, char *const argv[], int in_fd, int out_fd);
/* Non-zero while running; reaps it once it has exited. */
int  child_process_running(ChildProcess *child);
/* Sends `sig` to the group, then waits up to timeout_ms before SIGKILL. */
void child_process_stop(ChildProcess *child, int sig, int timeout_ms);

#endif
