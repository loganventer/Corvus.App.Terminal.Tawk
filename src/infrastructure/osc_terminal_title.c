#include "infrastructure/osc_terminal_title.h"
#include "utilities/str_util.h"

#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

typedef struct OscTitle {
    int              fd;
    int              windows_terminal;
    TerminalProgress last_progress;
} OscTitle;

static void write_all(int fd, const char *s, size_t len) {
    while (fd >= 0 && len > 0) {
        ssize_t n = write(fd, s, len);
        if (n <= 0) return;
        s += n;
        len -= (size_t)n;
    }
}

static void title_set(ITerminalTitle *self, const char *title) {
    OscTitle *t = self->ctx;
    char clean[512];
    str_copy(clean, sizeof(clean), title);
    str_strip_controls(clean);   /* remote text must never terminate the escape early */
    char seq[600];
    int n = snprintf(seq, sizeof(seq), "\033]0;%s\007", clean);
    if (n > 0) write_all(t->fd, seq, (size_t)n < sizeof(seq) ? (size_t)n : sizeof(seq) - 1);
}

static void title_alert(ITerminalTitle *self) {
    write_all(((OscTitle *)self->ctx)->fd, "\007", 1);
}

/* OSC 9;4 is Windows Terminal's tab progress ring. Other terminals (iTerm2)
 * read OSC 9 as a desktop notification, so it is only sent inside WT. */
static void title_progress(ITerminalTitle *self, TerminalProgress state) {
    OscTitle *t = self->ctx;
    if (!t->windows_terminal || state == t->last_progress) return;
    t->last_progress = state;
    const char *seq = state == TERMINAL_PROGRESS_BUSY ? "\033]9;4;3;0\007"
                    : state == TERMINAL_PROGRESS_ERROR ? "\033]9;4;2;100\007" : "\033]9;4;0;0\007";
    write_all(t->fd, seq, strlen(seq));
}

static void title_destroy(ITerminalTitle *self) {
    if (!self) return;
    OscTitle *t = self->ctx;
    title_progress(self, TERMINAL_PROGRESS_NONE);
    write_all(t->fd, "\033[23;0t", 7);   /* pop saved title */
    if (t->fd >= 0) close(t->fd);
    free(t);
    free(self);
}

ITerminalTitle *osc_terminal_title_create(void) {
    ITerminalTitle *tt = calloc(1, sizeof(*tt));
    OscTitle *t = calloc(1, sizeof(*t));
    if (!tt || !t) { free(tt); free(t); return NULL; }
    t->fd = open("/dev/tty", O_WRONLY | O_CLOEXEC | O_NOCTTY);
    t->windows_terminal = getenv("WT_SESSION") != NULL;
    write_all(t->fd, "\033[22;0t", 7);   /* push current title */
    tt->ctx = t;
    tt->set = title_set;
    tt->alert = title_alert;
    tt->progress = title_progress;
    tt->destroy = title_destroy;
    return tt;
}
