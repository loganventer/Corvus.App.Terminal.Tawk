#include "clients/tui/private_unlock_dialog.h"
#include "clients/tui/tui_draw.h"
#include "clients/tui/tui_palette.h"
#include "utilities/secure_zero.h"

#include <ncurses.h>
#include <stdio.h>
#include <string.h>

#define DOT " \xC2\xB7 "

void private_unlock_dialog_open(PrivateUnlockDialog *d, UnlockPurpose purpose, int touch_id) {
    secure_zero(d, sizeof(*d));
    d->purpose = purpose;
    d->state = touch_id ? PRIVATE_UNLOCK_WAITING_TOUCH_ID : PRIVATE_UNLOCK_PASSWORD;
    d->open = 1;
}

void private_unlock_dialog_show_password(PrivateUnlockDialog *d) {
    masked_field_wipe(&d->field);
    d->state = PRIVATE_UNLOCK_PASSWORD;
}

void private_unlock_dialog_wrong(PrivateUnlockDialog *d, int64_t wait_ms, int64_t now_ms) {
    masked_field_wipe(&d->field);
    d->wrong = 1;
    d->retry_at_ms = wait_ms > 0 ? now_ms + wait_ms : 0;
    d->state = wait_ms > 0 ? PRIVATE_UNLOCK_WAIT : PRIVATE_UNLOCK_PASSWORD;
}

void private_unlock_dialog_take(PrivateUnlockDialog *d, Passphrase *out) {
    masked_field_take(&d->field, out);
}

void private_unlock_dialog_close(PrivateUnlockDialog *d) {
    secure_zero(d, sizeof(*d));
}

static void settle_wait(PrivateUnlockDialog *d, int64_t now_ms) {
    if (d->state == PRIVATE_UNLOCK_WAIT && now_ms >= d->retry_at_ms) d->state = PRIVATE_UNLOCK_PASSWORD;
}

PrivateUnlockKey private_unlock_dialog_key(PrivateUnlockDialog *d, int is_key, int ch, int64_t now_ms) {
    if (!d->open) return PRIVATE_UNLOCK_KEY_NONE;
    settle_wait(d, now_ms);
    if (!is_key && ch == 27) return PRIVATE_UNLOCK_KEY_CANCEL;
    if (d->state == PRIVATE_UNLOCK_WAITING_TOUCH_ID) {
        if ((!is_key && ch == '\t') || (is_key && ch == KEY_BTAB)) return PRIVATE_UNLOCK_KEY_USE_PASSWORD;
        return PRIVATE_UNLOCK_KEY_NONE;                  /* everything else is swallowed while the sheet is up */
    }
    if ((is_key && ch == KEY_ENTER) || (!is_key && (ch == '\n' || ch == '\r'))) {
        if (d->state == PRIVATE_UNLOCK_WAIT || d->field.length == 0) return PRIVATE_UNLOCK_KEY_CHANGED;
        return PRIVATE_UNLOCK_KEY_SUBMIT;
    }
    if (masked_field_key(&d->field, is_key, ch)) {
        if (d->state == PRIVATE_UNLOCK_PASSWORD) d->wrong = 0;      /* typing again clears "Wrong password" */
        return PRIVATE_UNLOCK_KEY_CHANGED;
    }
    return PRIVATE_UNLOCK_KEY_NONE;
}

void private_unlock_dialog_render(PrivateUnlockDialog *d, UiRect area, int64_t now_ms) {
    if (!d->open) return;
    settle_wait(d, now_ms);
    int w = area.w < 56 ? area.w : 56;
    int h = area.h < 9 ? area.h : 9;
    UiRect box = { area.y + (area.h - h) / 2, area.x + (area.w - w) / 2, h, w };
    d->last_rect = box;
    int base = tui_palette_attr(THEME_SLOT_BASE);
    int dim = tui_palette_attr(THEME_SLOT_DIM);
    tui_fill(box, base);
    tui_box(box, " \xF0\x9F\x94\x92 Private chat ", tui_palette_attr(THEME_SLOT_BORDER));

    char purpose[128];
    const char *reason = unlock_purpose_reason(d->purpose);
    snprintf(purpose, sizeof(purpose), "To %s", reason);
    tui_text_center(box.y + 2, box.x, box.w, purpose, base | ATTR_BOLD);

    char line[96], hint[96];
    if (d->state == PRIVATE_UNLOCK_WAITING_TOUCH_ID) {
        tui_text_center(box.y + 4, box.x, box.w, "Touch ID\xE2\x80\xA6", base);
        snprintf(hint, sizeof(hint), "Tab use the lock password" DOT "Esc cancel");
    } else {
        masked_field_mask(&d->field, 32, line, sizeof(line));
        tui_text_center(box.y + 4, box.x, box.w, line, base);
        if (d->state == PRIVATE_UNLOCK_WAIT) {
            char wait[64];
            snprintf(wait, sizeof(wait), "Wrong password. Try again in %d s", (int)((d->retry_at_ms - now_ms + 999) / 1000));
            tui_text_center(box.y + 5, box.x, box.w, wait, tui_palette_attr(THEME_SLOT_WARN));
        } else if (d->wrong) {
            tui_text_center(box.y + 5, box.x, box.w, "Wrong password", tui_palette_attr(THEME_SLOT_WARN));
        } else {
            tui_text_center(box.y + 5, box.x, box.w, "Type the lock password", dim);
        }
        snprintf(hint, sizeof(hint), "Enter unlock" DOT "Esc cancel");
    }
    tui_text_center(box.y + box.h - 2, box.x, box.w, hint, dim);
}
