#include "infrastructure/terminal_passphrase_prompt.h"
#include "utilities/secure_zero.h"

#include <fcntl.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <termios.h>
#include <unistd.h>

/* The terminal settings to put back if a signal arrives while echo is off. */
static int            s_tty = -1;
static struct termios s_saved;

static void restore_and_reraise(int sig) {
    if (s_tty >= 0) tcsetattr(s_tty, TCSAFLUSH, &s_saved);
    signal(sig, SIG_DFL);
    raise(sig);
}

/* One line from the terminal with echo off; returns its length or -1. */
static int read_hidden(int tty, const char *prompt, char *buf, size_t size) {
    if (write(tty, prompt, strlen(prompt)) < 0) return -1;
    struct termios quiet;
    if (tcgetattr(tty, &s_saved) != 0) return -1;
    quiet = s_saved;
    quiet.c_lflag &= ~(tcflag_t)(ECHO | ECHONL);
    quiet.c_lflag |= ICANON;
    s_tty = tty;
    void (*old_int)(int) = signal(SIGINT, restore_and_reraise);
    void (*old_term)(int) = signal(SIGTERM, restore_and_reraise);
    void (*old_hup)(int) = signal(SIGHUP, restore_and_reraise);
    tcsetattr(tty, TCSAFLUSH, &quiet);

    size_t used = 0;
    int ok = 0;
    for (;;) {
        char c;
        ssize_t n = read(tty, &c, 1);
        if (n <= 0) break;
        if (c == '\n' || c == '\r') { ok = 1; break; }
        if (used + 1 < size) buf[used++] = c;
        else { ok = 0; used = size; }                    /* too long: keep reading to the end, then refuse */
    }
    tcsetattr(tty, TCSAFLUSH, &s_saved);
    s_tty = -1;
    signal(SIGINT, old_int);
    signal(SIGTERM, old_term);
    signal(SIGHUP, old_hup);
    if (write(tty, "\n", 1) < 0) {}
    if (!ok || used >= size) return -1;
    buf[used] = '\0';
    return (int)used;
}

static int ask(IPassphrasePrompt *self, const char *prompt, int confirm, Passphrase *out) {
    (void)self;
    int tty = open("/dev/tty", O_RDWR | O_CLOEXEC | O_NOCTTY);
    if (tty < 0) return -1;
    char first[PASSPHRASE_MAX + 2], second[PASSPHRASE_MAX + 2];
    int rc = -1;
    if (read_hidden(tty, prompt, first, sizeof(first)) > 0) {
        rc = passphrase_set(out, first);
        if (rc == 0 && confirm) {
            if (read_hidden(tty, "Type it again: ", second, sizeof(second)) < 0 || strcmp(first, second) != 0) {
                const char *msg = "The two passphrases differ.\n";
                if (write(tty, msg, strlen(msg)) < 0) {}
                passphrase_wipe(out);
                rc = -1;
            }
        }
    }
    secure_zero(first, sizeof(first));
    secure_zero(second, sizeof(second));
    close(tty);
    return rc;
}

static void destroy(IPassphrasePrompt *self) { free(self); }

IPassphrasePrompt *terminal_passphrase_prompt_create(void) {
    IPassphrasePrompt *p = calloc(1, sizeof(*p));
    if (!p) return NULL;
    p->ask = ask;
    p->destroy = destroy;
    return p;
}
