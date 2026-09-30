/* The screensaver must stop on a key press even when its command floods the
 * screen. On macOS an exiting process waits until its pty output has been
 * read, so tawk used to hang here forever. */
#include "contracts/i_idle_action.h"
#include "infrastructure/pty_idle_action.h"

#include <poll.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>

#if defined(__APPLE__)
#include <util.h>
#else
#include <pty.h>
#endif

static double now(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec + ts.tv_nsec / 1e9;
}

int main(void) {
    struct winsize ws = { 40, 120, 0, 0 };
    int term = -1;
    pid_t pid = forkpty(&term, NULL, NULL, &ws);
    if (pid < 0) { perror("forkpty"); return 1; }
    if (pid == 0) {
        IIdleAction *a = pty_idle_action_create();
        a->run(a, "yes 'falling code falling code falling code'", NULL, NULL);
        _exit(0);
    }

    char buf[65536];
    double start = now(), key_at = 0;
    int status = 0;
    for (;;) {
        struct pollfd p = { term, POLLIN, 0 };
        if (poll(&p, 1, 50) > 0 && read(term, buf, sizeof(buf)) <= 0) { /* the terminal closed */ }
        if (!key_at && now() - start > 1.0) {
            if (write(term, "x", 1) != 1) { perror("write"); return 1; }
            key_at = now();
        }
        if (waitpid(pid, &status, WNOHANG) == pid) break;
        if (key_at && now() - key_at > 5.0) {
            fprintf(stderr, "FAIL: the screensaver was still running 5 s after a key press\n");
            kill(pid, SIGKILL);
            return 1;
        }
    }
    printf("ok: the screensaver stopped %.2f s after a key press\n", now() - key_at);
    return 0;
}
