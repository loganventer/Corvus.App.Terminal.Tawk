#include "clients/tui/text_veil.h"
#include "clients/tui/tui_draw.h"
#include "utilities/utf8_text.h"

#include <ncurses.h>

#define LIGHT  "\xE2\x96\x91"   /* ░ */
#define MEDIUM "\xE2\x96\x92"   /* ▒ */

void text_veil_draw(int y, int x, int cols, int attr) {
    attrset(attr);
    for (int c = 0; c < cols; c++) {
        /* An uneven mix of shades reads as blurred rather than as a pattern. */
        mvaddstr(y, x + c, ((x + c) * 7 + y * 3) % 5 == 0 ? MEDIUM : LIGHT);
    }
    attrset(A_NORMAL);
}

void text_veil_text(int y, int x, int max_cols, const char *text, unsigned long length, int attr) {
    if (!text) return;
    int cols = 0;
    utf8_fit(text, length, max_cols, &cols);
    text_veil_draw(y, x, cols, attr);
}
