#include "utilities/process_capture.h"
#include "utilities/child_process.h"
#include "utilities/clock_util.h"

#include <poll.h>
#include <signal.h>
#include <unistd.h>

int process_run_to_fd(char *const argv[], int out_fd, int timeout_ms) {
    ChildProcess child;
    if (child_process_start_io(&child, argv, -1, out_fd) != 0) return -1;
    int64_t until = clock_now_ms() + timeout_ms;
    while (child_process_running(&child)) {
        if (clock_now_ms() > until) { child_process_stop(&child, SIGTERM, 200); return -1; }
        poll(NULL, 0, 20);
    }
    return 0;
}

int process_capture(char *const argv[], char *buf, size_t size, int timeout_ms) {
    int fds[2];
    if (size == 0 || pipe(fds) != 0) return -1;
    ChildProcess child;
    int rc = child_process_start_io(&child, argv, -1, fds[1]);
    close(fds[1]);
    size_t used = 0;
    if (rc == 0) {
        int64_t until = clock_now_ms() + timeout_ms;
        struct pollfd p = { fds[0], POLLIN, 0 };
        while (clock_now_ms() < until && poll(&p, 1, 100) >= 0) {
            if (!(p.revents & (POLLIN | POLLHUP))) { if (!child_process_running(&child)) break; continue; }
            ssize_t n = read(fds[0], buf + used, size - 1 - used);
            if (n <= 0) break;
            used += (size_t)n;
            if (used + 1 >= size) break;
        }
        child_process_stop(&child, SIGTERM, 200);
    }
    close(fds[0]);
    buf[used] = '\0';
    return rc;
}
