#include "clients/tui/tui_draw.h"
#include "clients/tui/tui_palette.h"
#include "utilities/utf8_text.h"

#include <ncurses.h>
#include <stdio.h>
#include <string.h>

void tui_fill(UiRect r, int attr) {
    /* The colour must be part of the fill character: hline ignores attrset
     * and would paint the window background, leaving colour only behind text. */
    chtype blank = (chtype)' ' | (chtype)attr;
    for (int row = 0; row < r.h; row++) mvhline(r.y + row, r.x, blank, r.w);
}

/* Writes text without U+FE0F. Terminals such as macOS Terminal and iTerm2
 * draw a symbol followed by it (❤️, ⚠️) two columns wide while curses and our
 * layout count one, so the next character lands on top of it. Without the
 * selector every terminal draws the symbol in its one-column form. */
static void add_without_vs16(const char *text, size_t bytes) {
    size_t start = 0;
    for (size_t i = 0; i + 2 < bytes; i++) {
        if ((unsigned char)text[i] == 0xEF && (unsigned char)text[i + 1] == 0xB8 &&
            (unsigned char)text[i + 2] == 0x8F) {
            if (i > start) addnstr(text + start, (int)(i - start));
            start = i + 3;
            i += 2;
        }
    }
    if (bytes > start) addnstr(text + start, (int)(bytes - start));
}

int tui_text_n(int y, int x, int max_cols, const char *text, unsigned long len, int attr) {
    if (!text || max_cols <= 0) return 0;
    int cols = 0;
    size_t bytes = utf8_fit(text, len, max_cols, &cols);
    attrset(attr);
    move(y, x);
    add_without_vs16(text, bytes);
    attrset(A_NORMAL);
    return cols;
}

int tui_text(int y, int x, int max_cols, const char *text, int attr) {
    return text ? tui_text_n(y, x, max_cols, text, strlen(text), attr) : 0;
}

int tui_text_right(int y, int right_x, int max_cols, const char *text, int attr) {
    int cols = utf8_columns(text);
    if (cols > max_cols) cols = max_cols;
    return tui_text(y, right_x - cols, cols, text, attr);
}

void tui_text_center(int y, int x, int w, const char *text, int attr) {
    int cols = utf8_columns(text);
    if (cols > w) cols = w;
    tui_text(y, x + (w - cols) / 2, w, text, attr);
}

/* Unicode box drawing instead of ACS: every UTF-8 terminal renders it,
 * including tmux and Windows Terminal, without alternate-charset quirks. */
static void draw_hrule(int y, int x, int n, const char *glyph) {
    for (int i = 0; i < n; i++) mvaddstr(y, x + i, glyph);
}

void tui_vline(int y, int x, int n, int attr) {
    attrset(attr);
    for (int i = 0; i < n; i++) mvaddstr(y + i, x, "\xE2\x94\x82");
    attrset(A_NORMAL);
}

void tui_box(UiRect r, const char *title, int attr) {
    if (r.h < 2 || r.w < 2) return;
    tui_fill(r, attr);
    attrset(attr);
    draw_hrule(r.y, r.x + 1, r.w - 2, "\xE2\x94\x80");
    draw_hrule(r.y + r.h - 1, r.x + 1, r.w - 2, "\xE2\x94\x80");
    for (int i = 1; i < r.h - 1; i++) {
        mvaddstr(r.y + i, r.x, "\xE2\x94\x82");
        mvaddstr(r.y + i, r.x + r.w - 1, "\xE2\x94\x82");
    }
    mvaddstr(r.y, r.x, "\xE2\x95\xAD");
    mvaddstr(r.y, r.x + r.w - 1, "\xE2\x95\xAE");
    mvaddstr(r.y + r.h - 1, r.x, "\xE2\x95\xB0");
    mvaddstr(r.y + r.h - 1, r.x + r.w - 1, "\xE2\x95\xAF");
    attrset(A_NORMAL);
    if (title && *title) {
        char padded[160];
        snprintf(padded, sizeof(padded), " %s ", title);
        tui_text(r.y, r.x + 2, r.w - 4, padded, attr | ATTR_BOLD);
    }
}
