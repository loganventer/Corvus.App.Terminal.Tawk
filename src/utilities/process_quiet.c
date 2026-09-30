#include "utilities/process_quiet.h"

#include <errno.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

int process_run_quiet(char *const argv[], const void *secret, size_t secret_len, int secret_fd) {
    int fds[2] = { -1, -1 };
    if (secret && pipe(fds) != 0) return -1;
    pid_t pid = fork();
    if (pid < 0) {
        if (secret) { close(fds[0]); close(fds[1]); }
        return -1;
    }
    if (pid == 0) {
        int null = open("/dev/null", O_RDWR);
        if (null >= 0) { dup2(null, 0); dup2(null, 1); dup2(null, 2); }
        if (secret) {
            close(fds[1]);
            if (fds[0] != secret_fd) { dup2(fds[0], secret_fd); close(fds[0]); }
        }
        execvp(argv[0], argv);
        _exit(127);
    }
    if (secret) {
        close(fds[0]);
        /* A passphrase fits in the pipe's buffer, so this never waits on a
         * child that stops reading. */
        const char *p = secret;
        size_t left = secret_len;
        while (left > 0) {
            ssize_t n = write(fds[1], p, left);
            if (n < 0 && errno == EINTR) continue;
            if (n <= 0) break;
            p += n;
            left -= (size_t)n;
        }
        close(fds[1]);
    }
    int status = 0;
    while (waitpid(pid, &status, 0) < 0) if (errno != EINTR) return -1;
    return WIFEXITED(status) ? WEXITSTATUS(status) : -1;
}
