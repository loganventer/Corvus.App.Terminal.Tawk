#include "infrastructure/pty_idle_action.h"
#include "utilities/log.h"

#include <errno.h>
#include <poll.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/wait.h>
#include <termios.h>
#include <time.h>
#include <unistd.h>

#if defined(__APPLE__)
#include <util.h>
#else
#include <pty.h>
#endif

static void stop_child(pid_t pid) {
    kill(-pid, SIGTERM);
    for (int i = 0; i < 50; i++) {
        if (waitpid(pid, NULL, WNOHANG) == pid) return;
        struct timespec ts = { 0, 20 * 1000000L };
        nanosleep(&ts, NULL);
    }
    kill(-pid, SIGKILL);
    waitpid(pid, NULL, 0);
}

static int idle_run(IIdleAction *self, const char *command, IdleWakeCheck wake_check, void *user) {
    (void)self;
    if (!command || !*command) return -1;
    struct winsize ws;
    if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) != 0) memset(&ws, 0, sizeof(ws));

    struct termios saved, raw;
    int have_termios = tcgetattr(STDIN_FILENO, &saved) == 0;
    int master = -1;
    pid_t pid = forkpty(&master, NULL, NULL, &ws);
    if (pid < 0) return -1;
    if (pid == 0) {
        /* The command comes from the user's own (trusted) config file. */
        execl("/bin/sh", "sh", "-c", command, (char *)NULL);
        _exit(127);
    }

    if (have_termios) {
        raw = saved;
        cfmakeraw(&raw);
        tcsetattr(STDIN_FILENO, TCSANOW, &raw);
    }

    struct pollfd fds[2] = { { STDIN_FILENO, POLLIN, 0 }, { master, POLLIN, 0 } };
    char buf[8192];
    int child_done = 0;
    for (;;) {
        int n = poll(fds, 2, 100);
        if (n < 0 && errno != EINTR) break;
        if (n > 0 && (fds[0].revents & POLLIN)) break;             /* any key or mouse input */
        if (n > 0 && (fds[1].revents & (POLLIN | POLLHUP))) {
            ssize_t got = read(master, buf, sizeof(buf));
            if (got <= 0) { child_done = 1; break; }
            ssize_t off = 0;
            while (off < got) {
                ssize_t w = write(STDOUT_FILENO, buf + off, (size_t)(got - off));
                if (w <= 0) break;
                off += w;
            }
        }
        struct winsize now;
        if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &now) == 0 && (now.ws_row != ws.ws_row || now.ws_col != ws.ws_col)) {
            ws = now;
            ioctl(master, TIOCSWINSZ, &ws);
            kill(-pid, SIGWINCH);
        }
        if (wake_check && wake_check(user)) break;
        if (waitpid(pid, NULL, WNOHANG) == pid) { child_done = 1; break; }
    }

    /* Close our end first: on macOS an exiting process waits until its pty
     * output has been read, and we stopped reading, so a command that draws
     * a lot (matrix-clock) would never exit. Closing discards that output. */
    close(master);
    if (!child_done) stop_child(pid);
    if (have_termios) tcsetattr(STDIN_FILENO, TCSANOW, &saved);
    tcflush(STDIN_FILENO, TCIFLUSH);                               /* swallow the wake key */
    /* Leave the alternate screen and reset attributes the command may have left behind. */
    static const char reset[] = "\033[?1049l\033[0m\033[?25h";
    ssize_t ignored = write(STDOUT_FILENO, reset, sizeof(reset) - 1);
    (void)ignored;
    return 0;
}

static void idle_destroy(IIdleAction *self) { free(self); }

IIdleAction *pty_idle_action_create(void) {
    IIdleAction *a = calloc(1, sizeof(*a));
    if (!a) return NULL;
    a->run = idle_run;
    a->destroy = idle_destroy;
    return a;
}
