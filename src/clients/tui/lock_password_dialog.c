#include "clients/tui/lock_password_dialog.h"
#include "clients/tui/tui_draw.h"
#include "clients/tui/tui_palette.h"
#include "engines/lock_password_rule.h"
#include "utilities/secure_zero.h"

#include <ncurses.h>
#include <stdio.h>

#define DOT " \xC2\xB7 "

void lock_password_dialog_open(LockPasswordDialog *d, int first) {
    secure_zero(d, sizeof(*d));
    d->first = first;
    d->open = 1;
}

void lock_password_dialog_close(LockPasswordDialog *d) {
    secure_zero(d, sizeof(*d));
}

void lock_password_dialog_take(LockPasswordDialog *d, Passphrase *password, Passphrase *confirm) {
    masked_field_take(&d->fields[0], password);
    masked_field_take(&d->fields[1], confirm);
    d->focus = 0;
}

void lock_password_dialog_refused(LockPasswordDialog *d, LockPasswordProblem problem) {
    masked_field_wipe(&d->fields[0]);
    masked_field_wipe(&d->fields[1]);
    d->problem = problem;
    d->focus = 0;
}

LockPasswordKey lock_password_dialog_key(LockPasswordDialog *d, int is_key, int ch) {
    if (!d->open) return LOCK_PASSWORD_KEY_NONE;
    if (!is_key && ch == 27) return LOCK_PASSWORD_KEY_CANCEL;
    if ((!is_key && ch == '\t') || (is_key && (ch == KEY_BTAB || ch == KEY_DOWN || ch == KEY_UP))) {
        d->focus = !d->focus;
        return LOCK_PASSWORD_KEY_CHANGED;
    }
    if ((is_key && ch == KEY_ENTER) || (!is_key && (ch == '\n' || ch == '\r'))) {
        if (d->focus == 0) { d->focus = 1; return LOCK_PASSWORD_KEY_CHANGED; }
        return LOCK_PASSWORD_KEY_SUBMIT;
    }
    if (masked_field_key(&d->fields[d->focus], is_key, ch)) { d->problem = LOCK_PASSWORD_OK; return LOCK_PASSWORD_KEY_CHANGED; }
    return LOCK_PASSWORD_KEY_NONE;
}

void lock_password_dialog_render(LockPasswordDialog *d, UiRect area) {
    if (!d->open) return;
    int w = area.w < 60 ? area.w : 60;
    int h = area.h < 11 ? area.h : 11;
    UiRect box = { area.y + (area.h - h) / 2, area.x + (area.w - w) / 2, h, w };
    d->last_rect = box;
    int base = tui_palette_attr(THEME_SLOT_BASE);
    int dim = tui_palette_attr(THEME_SLOT_DIM);
    int sel = tui_palette_attr(THEME_SLOT_SIDEBAR_SELECTED) | ATTR_BOLD;
    tui_fill(box, base);
    tui_box(box, d->first ? " \xF0\x9F\x94\x92 Set a lock password " : " \xF0\x9F\x94\x92 Change the lock password ",
            tui_palette_attr(THEME_SLOT_BORDER));
    tui_text_center(box.y + 2, box.x, box.w,
                    d->first ? "Private chats open with this, or Touch ID" : "The new password for private chats", base | ATTR_BOLD);

    const char *labels[2] = { "New password ", "Same again   " };
    for (int i = 0; i < 2; i++) {
        char mask[48], row[96];
        masked_field_mask(&d->fields[i], 30, mask, sizeof(mask));
        snprintf(row, sizeof(row), "%s %s", labels[i], mask);
        tui_text(box.y + 4 + i, box.x + 4, box.w - 8, row, d->focus == i ? sel : base);
    }
    if (d->problem != LOCK_PASSWORD_OK)
        tui_text_center(box.y + 7, box.x, box.w, lock_password_problem_text(d->problem), tui_palette_attr(THEME_SLOT_WARN));
    tui_text_center(box.y + box.h - 2, box.x, box.w, "Tab next field" DOT "Enter set" DOT "Esc cancel", dim);
}
