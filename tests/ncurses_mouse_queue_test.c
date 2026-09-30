/* tawk reads mouse events by calling getmouse() until the queue is empty,
 * because one KEY_MOUSE can stand for several events (a press and its
 * release, or clicks that arrived together). This checks that no click is
 * lost and that the queue then reports empty. ncurses differs by version:
 * 6.6 (Homebrew) reports a press and a release, oldest first; 6.4 (Ubuntu
 * 24.04) merges them into one "clicked" event and returns the newest first.
 * tawk acts on either, so the test accepts both and ignores the order. */
#include <ncurses.h>
#include <poll.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

#if defined(__APPLE__)
#include <util.h>
#else
#include <pty.h>
#endif

/* What the terminal sends (SGR mouse reports) and the clicks tawk must see. */
static const char INPUT[] =
    "\033[<2;20;5M\033[<2;20;5m"      /* right-click */
    "\033[<0;30;6M\033[<0;30;6m"      /* then a left click, in the same read */
    "\033[<2;22;7M\033[<2;22;7m";     /* then another right-click */
static const char *const EXPECTED[] = { "right 4,19", "left 5,29", "right 6,21" };
#define CLICKS (sizeof(EXPECTED) / sizeof(EXPECTED[0]))

static void child(int out) {
    initscr();
    cbreak();
    noecho();
    keypad(stdscr, TRUE);
    mousemask(ALL_MOUSE_EVENTS | REPORT_MOUSE_POSITION, NULL);
    mouseinterval(0);
    timeout(2000);
    char line[64];
    for (;;) {
        int ch = getch();
        if (ch == ERR) break;
        if (ch != KEY_MOUSE) continue;
        MEVENT ev;
        for (int i = 0; i < 64 && getmouse(&ev) == OK; i++) {
            /* What tawk acts on: a press or a "clicked" event. Releases are ignored. */
            const char *what = (ev.bstate & (BUTTON3_PRESSED | BUTTON3_CLICKED)) ? "right" :
                               (ev.bstate & (BUTTON1_PRESSED | BUTTON1_CLICKED)) ? "left" : NULL;
            if (!what) continue;
            int n = snprintf(line, sizeof(line), "%s %d,%d\n", what, ev.y, ev.x);
            if (write(out, line, (size_t)n) != n) break;
        }
    }
    endwin();
}

static size_t count_lines(const char *s) {
    size_t n = 0;
    for (; *s; s++) n += *s == '\n';
    return n;
}

int main(void) {
    int fds[2];
    if (pipe(fds) != 0) { perror("pipe"); return 1; }
    struct winsize ws = { 40, 120, 0, 0 };
    int term = -1;
    pid_t pid = forkpty(&term, NULL, NULL, &ws);
    if (pid < 0) { perror("forkpty"); return 1; }
    if (pid == 0) {
        close(fds[0]);
        setenv("TERM", "xterm-256color", 1);
        child(fds[1]);
        _exit(0);
    }
    close(fds[1]);

    char buf[4096], got[1024] = "";
    size_t used = 0;
    int sent = 0;
    for (int ms = 0; ms < 6000 && count_lines(got) < CLICKS; ms += 50) {
        struct pollfd p[2] = { { term, POLLIN, 0 }, { fds[0], POLLIN, 0 } };
        poll(p, 2, 50);
        if (p[0].revents & POLLIN) { if (read(term, buf, sizeof(buf)) <= 0) break; }
        if ((p[1].revents & POLLIN) && used + 1 < sizeof(got)) {
            ssize_t n = read(fds[0], got + used, sizeof(got) - 1 - used);
            if (n > 0) { used += (size_t)n; got[used] = '\0'; }
        }
        if (!sent && ms >= 500) {
            if (write(term, INPUT, sizeof(INPUT) - 1) != (ssize_t)(sizeof(INPUT) - 1)) { perror("write"); return 1; }
            sent = 1;
        }
    }
    kill(pid, SIGKILL);
    waitpid(pid, NULL, 0);

    /* Each expected click exactly once, in any order, and nothing else. */
    int ok = count_lines(got) == CLICKS;
    for (size_t i = 0; ok && i < CLICKS; i++) {
        char want[32];
        snprintf(want, sizeof(want), "%s\n", EXPECTED[i]);
        const char *hit = strstr(got, want);
        ok = hit && (hit == got || hit[-1] == '\n');
    }
    if (!ok) {
        fprintf(stderr, "FAIL: mouse clicks lost\n  expected, in any order: right 4,19 / left 5,29 / right 6,21\n  got:\n%s", got);
        return 1;
    }
    printf("ok: every queued mouse click arrived\n");
    return 0;
}
