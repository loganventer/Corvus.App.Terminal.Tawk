#include "clients/tui/lock_screen.h"
#include "clients/tui/tui_palette.h"
#include "engines/idle_lock_policy.h"
#include <ncurses.h>

#include <stdio.h>
#include <string.h>

static void forget(LockScreen *l) {
    masked_field_wipe(&l->field);
}

void lock_screen_lock(LockScreen *l) {
    forget(l);
    l->locked = 1;
    l->failed = 0;
    l->retry_at_ms = 0;
}

void lock_screen_unlock(LockScreen *l) {
    forget(l);
    l->locked = 0;
    l->failed = 0;
    l->retry_at_ms = 0;
}

void lock_screen_take(LockScreen *l, Passphrase *out) {
    masked_field_take(&l->field, out);
}

void lock_screen_wrong(LockScreen *l, int64_t now_ms) {
    l->failed++;
    l->retry_at_ms = now_ms + idle_lock_policy_wait_ms(l->failed);
}

LockKey lock_screen_key(LockScreen *l, int is_key, int ch, int64_t now_ms) {
    if (!l->locked) return LOCK_KEY_NONE;
    if ((is_key && ch == KEY_ENTER) || (!is_key && (ch == '\n' || ch == '\r'))) {
        if (now_ms < l->retry_at_ms || l->field.length == 0) return LOCK_KEY_CHANGED;
        return LOCK_KEY_SUBMIT;
    }
    return masked_field_key(&l->field, is_key, ch) ? LOCK_KEY_CHANGED : LOCK_KEY_NONE;
}

static void centred(int y, int cols, const char *text, int attr) {
    int len = (int)strlen(text);
    int x = cols > len ? (cols - len) / 2 : 0;
    attron(attr);
    mvaddnstr(y, x, text, cols);
    attroff(attr);
}

void lock_screen_draw(const LockScreen *l, int rows, int cols, int64_t now_ms) {
    erase();
    int y = rows / 2 - 2;
    if (y < 0) y = 0;
    centred(y, cols, "tawk is locked", ATTR_BOLD);
    centred(y + 2, cols, "Type the passphrase of your chats and press Enter", ATTR_DIM);
    /* How much was typed, never what. */
    char dots[64];
    masked_field_mask(&l->field, 40, dots, sizeof(dots));
    centred(y + 4, cols, dots, 0);
    if (now_ms < l->retry_at_ms) {
        char wait[96];
        snprintf(wait, sizeof(wait), "That was not it. Try again in %d s", (int)((l->retry_at_ms - now_ms + 999) / 1000));
        centred(y + 6, cols, wait, ATTR_DIM);
    } else if (l->failed > 0) {
        centred(y + 6, cols, "That was not it", ATTR_DIM);
    }
    refresh();
}
