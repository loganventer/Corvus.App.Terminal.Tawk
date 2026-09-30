#include "clients/tui/profile_text_dialog.h"
#include "clients/tui/tui_draw.h"
#include "clients/tui/tui_palette.h"
#include "utilities/str_util.h"
#include "utilities/utf8_text.h"

#include <ncurses.h>
#include <stdio.h>
#include <string.h>

#define WIDTH 60

/* The name fits on one row; the about text gets a few. */
static int input_rows(ProfileField field) { return field == PROFILE_FIELD_ABOUT ? 4 : 1; }

void profile_text_dialog_open(ProfileTextDialog *d, ProfileField field, const char *current, int max_chars) {
    memset(d, 0, sizeof(*d));
    d->field = field;
    text_field_init(&d->input, max_chars);
    text_field_set(&d->input, current ? current : "");
    d->input.cursor = d->input.length;
    d->open = 1;
}

void profile_text_dialog_close(ProfileTextDialog *d) { d->open = 0; }

PopupResult profile_text_dialog_key(ProfileTextDialog *d, int is_key, int ch) {
    if (!is_key && ch == 27) { d->open = 0; return POPUP_CLOSED; }
    if ((!is_key && (ch == '\n' || ch == '\r')) || (is_key && ch == KEY_ENTER)) return POPUP_CHOSEN;
    if (text_field_key(&d->input, is_key, ch)) { d->error[0] = '\0'; return POPUP_CHANGED; }
    return POPUP_NONE;
}

PopupResult profile_text_dialog_click(ProfileTextDialog *d, int y, int x) {
    if (ui_rect_contains(d->save_button, y, x)) return POPUP_CHOSEN;
    if (ui_rect_contains(d->cancel_button, y, x) || !ui_rect_contains(d->last_rect, y, x)) { d->open = 0; return POPUP_CLOSED; }
    return POPUP_NONE;
}

void  profile_text_dialog_paste(ProfileTextDialog *d, const char *utf8) { text_field_paste(&d->input, utf8); d->error[0] = '\0'; }
char *profile_text_dialog_text(const ProfileTextDialog *d) { return text_field_text(&d->input); }
void  profile_text_dialog_error(ProfileTextDialog *d, const char *why) { str_copy(d->error, sizeof(d->error), why ? why : ""); }

void profile_text_dialog_render(ProfileTextDialog *d, UiRect a, TextCaret *caret) {
    int rows = input_rows(d->field);
    int w = a.w < WIDTH ? a.w : WIDTH;
    int h = rows + 8;
    if (h > a.h) h = a.h;
    UiRect box = { a.y + (a.h - h) / 2, a.x + (a.w - w) / 2, h, w };
    d->last_rect = box;
    int base = tui_palette_attr(THEME_SLOT_BASE);
    char title[48];
    snprintf(title, sizeof(title), "Your %s", d->field == PROFILE_FIELD_NAME ? "name" : "about");
    tui_fill(box, base);
    tui_box(box, title, tui_palette_attr(THEME_SLOT_BORDER));

    /* The text sits in a framed box so it reads as editable. */
    UiRect frame = { box.y + 2, box.x + 2, rows + 2, box.w - 4 };
    tui_box(frame, NULL, tui_palette_attr(THEME_SLOT_ACCENT));
    UiRect inside = { frame.y + 1, frame.x + 1, rows, frame.w - 2 };
    text_field_render(&d->input, inside, tui_palette_attr(THEME_SLOT_COMPOSER), 1, caret);

    char count[32];
    snprintf(count, sizeof(count), "%d/%d", text_field_length(&d->input), d->input.max_chars);
    int y = frame.y + frame.h;
    tui_text_right(y, frame.x + frame.w, 12, count, base | ATTR_DIM);
    if (d->error[0]) tui_text(y, frame.x, frame.w - 12, d->error, tui_palette_attr(THEME_SLOT_WARN) | ATTR_BOLD);

    const char *cancel = "  Cancel  ", *save = "  Save  ";
    int cw = utf8_columns(cancel), sw = utf8_columns(save);
    int bx = box.x + (box.w - cw - sw - 4) / 2, by = box.y + box.h - 2;
    tui_text(by, bx, cw, cancel, base);
    tui_text(by, bx + cw + 4, sw, save, tui_palette_attr(THEME_SLOT_SIDEBAR_SELECTED) | ATTR_BOLD);
    d->cancel_button = (UiRect){ by, bx, 1, cw };
    d->save_button = (UiRect){ by, bx + cw + 4, 1, sw };
}
