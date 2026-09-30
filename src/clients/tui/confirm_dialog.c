#include "clients/tui/confirm_dialog.h"
#include "clients/tui/tui_draw.h"
#include "clients/tui/tui_palette.h"
#include "utilities/str_util.h"
#include "utilities/utf8_text.h"

#include <ncurses.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define FLASH_MS 450

void confirm_dialog_open(ConfirmDialog *d, ConfirmPurpose purpose, const char *subject, const char *title,
                         const char *question, const char *warning, const char *confirm_label, int danger) {
    memset(d, 0, sizeof(*d));
    d->purpose = purpose;
    str_copy(d->subject, sizeof(d->subject), subject ? subject : "");
    str_copy(d->title, sizeof(d->title), title ? title : "");
    str_copy(d->question, sizeof(d->question), question ? question : "");
    str_copy(d->warning, sizeof(d->warning), warning ? warning : "");
    str_copy(d->confirm_label, sizeof(d->confirm_label), confirm_label ? confirm_label : "OK");
    d->danger = danger;
    d->confirm_selected = 0;                      /* the safe choice first */
    d->open = 1;
}

static PopupResult finish(ConfirmDialog *d, int confirmed) {
    d->open = 0;
    return confirmed ? POPUP_CHOSEN : POPUP_CLOSED;
}

PopupResult confirm_dialog_key(ConfirmDialog *d, int is_key, int ch) {
    if (!is_key && (ch == 27 || ch == 'n' || ch == 'N')) return finish(d, 0);
    if (!is_key && (ch == 'y' || ch == 'Y')) return finish(d, 1);
    if ((is_key && (ch == KEY_LEFT || ch == KEY_RIGHT || ch == KEY_BTAB)) || (!is_key && ch == '\t')) {
        d->confirm_selected = !d->confirm_selected;
        return POPUP_CHANGED;
    }
    if ((!is_key && (ch == '\n' || ch == '\r')) || (is_key && ch == KEY_ENTER)) return finish(d, d->confirm_selected);
    return POPUP_NONE;
}

PopupResult confirm_dialog_click(ConfirmDialog *d, int y, int x) {
    if (ui_rect_contains(d->confirm_button, y, x)) return finish(d, 1);
    if (ui_rect_contains(d->cancel_button, y, x)) return finish(d, 0);
    if (!ui_rect_contains(d->last_rect, y, x)) return finish(d, 0);
    return POPUP_NONE;
}

/* Wraps `text` into lines of at most `cols` columns, drawing each centred. */
static int draw_wrapped(int y, UiRect box, const char *text, int attr) {
    TextLine *lines = NULL;
    int n = utf8_wrap(text, box.w - 6, &lines);
    for (int i = 0; i < n; i++) {
        char line[512];
        size_t len = lines[i].length < sizeof(line) - 1 ? lines[i].length : sizeof(line) - 1;
        memcpy(line, text + lines[i].offset, len);
        line[len] = '\0';
        tui_text_center(y + i, box.x, box.w, line, attr);
    }
    free(lines);
    return n;
}

void confirm_dialog_render(ConfirmDialog *d, UiRect area, int64_t now_ms) {
    int w = area.w < 64 ? area.w : 64;
    int lit = d->danger && (now_ms / FLASH_MS) % 2 == 0;
    int base = tui_palette_attr(THEME_SLOT_BASE);
    int warn = tui_palette_attr(THEME_SLOT_WARN) | ATTR_BOLD;
    int border = d->danger ? (lit ? warn | ATTR_REVERSE : warn) : tui_palette_attr(THEME_SLOT_BORDER);

    /* Height: title gap, question, gap, warning lines, gap, buttons. */
    TextLine *probe = NULL;
    int warn_lines = d->warning[0] ? utf8_wrap(d->warning, w - 6, &probe) : 0;
    free(probe);
    int h = 7 + warn_lines + (warn_lines ? 1 : 0);
    if (h > area.h) h = area.h;
    UiRect box = { area.y + (area.h - h) / 2, area.x + (area.w - w) / 2, h, w };
    d->last_rect = box;
    tui_fill(box, base);
    tui_box(box, d->title, border);

    int y = box.y + 2;
    tui_text_center(y, box.x, box.w, d->question, base | ATTR_BOLD);
    y += 2;
    if (warn_lines) {
        int attr = d->danger ? (lit ? warn | ATTR_REVERSE : warn) : tui_palette_attr(THEME_SLOT_DIM);
        y += draw_wrapped(y, box, d->warning, attr) + 1;
    }

    /* Buttons: Cancel on the left, the action on the right. */
    char cancel[32], confirm[64];
    snprintf(cancel, sizeof(cancel), "  Cancel  ");
    snprintf(confirm, sizeof(confirm), "  %s  ", d->confirm_label);
    int cw = utf8_columns(cancel), aw = utf8_columns(confirm);
    int bx = box.x + (box.w - cw - aw - 4) / 2, by = box.y + box.h - 2;
    int sel = tui_palette_attr(THEME_SLOT_SIDEBAR_SELECTED) | ATTR_BOLD;
    int action_attr = d->confirm_selected ? (d->danger ? warn | ATTR_REVERSE : sel) : (d->danger ? warn : base);
    tui_text(by, bx, cw, cancel, d->confirm_selected ? base : sel);
    tui_text(by, bx + cw + 4, aw, confirm, action_attr);
    d->cancel_button = (UiRect){ by, bx, 1, cw };
    d->confirm_button = (UiRect){ by, bx + cw + 4, 1, aw };
}
