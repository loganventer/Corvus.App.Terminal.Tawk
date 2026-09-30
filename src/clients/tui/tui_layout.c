#include "clients/tui/tui_layout.h"

#include <string.h>

#define MIN_CHAT_COLS   30

void tui_layout_compute(TuiLayout *l, int rows, int cols, int sidebar_width, int sidebar_collapsed,
                        int composer_text_rows) {
    memset(l, 0, sizeof(*l));
    if (rows < 6 || cols < 20) {
        l->body = (UiRect){ 0, 0, rows, cols };
        return;
    }
    l->header = (UiRect){ 0, 0, 1, cols };
    l->footer = (UiRect){ rows - 1, 0, 1, cols };
    l->body = (UiRect){ 1, 0, rows - 2, cols };

    int sb = sidebar_collapsed ? 0 : sidebar_width;
    if (sb > cols / 2) sb = cols / 2;
    if (cols - sb - 1 < MIN_CHAT_COLS) sb = 0;          /* narrow terminal: hide the list */
    int body_h = rows - 2;
    if (sb > 0) {
        l->sidebar = (UiRect){ 1, 0, body_h, sb };
        l->divider = (UiRect){ 1, sb, body_h, 1 };
    }
    int cx = sb > 0 ? sb + 1 : 0;
    /* One status line plus the input's text lines, leaving the conversation
     * at least a third of the body. */
    int text_rows = composer_text_rows < 1 ? 1 : composer_text_rows;
    int max_text = (body_h - 2) / 3;
    if (text_rows > max_text) text_rows = max_text < 1 ? 1 : max_text;
    int composer_h = 1 + text_rows;
    l->chat = (UiRect){ 1, cx, body_h - composer_h, cols - cx };
    l->composer = (UiRect){ 1 + body_h - composer_h, cx, composer_h, cols - cx };
}
