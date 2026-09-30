#include "clients/tui/text_field.h"
#include "clients/tui/tui_draw.h"
#include "clients/tui/tui_key_newline.h"
#include "utilities/utf8_text.h"

#include <ncurses.h>
#include <stdlib.h>
#include <string.h>

void text_field_init(TextField *f, int max_chars) {
    memset(f, 0, sizeof(*f));
    f->max_chars = max_chars > 0 && max_chars < TEXT_FIELD_CAPACITY ? max_chars : TEXT_FIELD_CAPACITY;
}

void text_field_allow_newlines(TextField *f, int allow) { f->multiline = allow; }

static int room(const TextField *f) { return f->length < f->max_chars; }

static void insert(TextField *f, wchar_t c) {
    if ((c < 32 && !(c == L'\n' && f->multiline)) || !room(f)) return;
    memmove(&f->text[f->cursor + 1], &f->text[f->cursor], sizeof(wchar_t) * (size_t)(f->length - f->cursor));
    f->text[f->cursor++] = c;
    f->text[++f->length] = L'\0';
}

static void erase_at(TextField *f, int at) {
    if (at < 0 || at >= f->length) return;
    memmove(&f->text[at], &f->text[at + 1], sizeof(wchar_t) * (size_t)(f->length - at));
    f->length--;
    if (f->cursor > at) f->cursor--;
}

void text_field_set(TextField *f, const char *utf8) {
    f->length = f->cursor = f->scroll_row = 0;
    f->text[0] = L'\0';
    text_field_paste(f, utf8);
}

void text_field_paste(TextField *f, const char *utf8) {
    if (!utf8) return;
    mbstate_t state;
    memset(&state, 0, sizeof(state));
    const char *p = utf8;
    size_t left = strlen(utf8);
    while (left > 0) {
        wchar_t c;
        size_t n = mbrtowc(&c, p, left, &state);
        if (n == (size_t)-1 || n == (size_t)-2) { p++; left--; memset(&state, 0, sizeof(state)); continue; }
        if (n == 0) break;
        if (c == L'\r') c = L'\n';
        insert(f, c == L'\t' || (c == L'\n' && !f->multiline) ? L' ' : c);   /* one paragraph unless multiline */
        p += n;
        left -= n;
    }
}

char *text_field_text(const TextField *f) { return utf8_from_wide(f->text, (size_t)f->length); }
int   text_field_length(const TextField *f) { return f->length; }

int text_field_key(TextField *f, int is_key, int ch) {
    if (is_key) {
        switch (ch) {
            case KEY_LEFT:  if (f->cursor > 0) f->cursor--; return 1;
            case KEY_RIGHT: if (f->cursor < f->length) f->cursor++; return 1;
            case KEY_HOME:  f->cursor = 0; return 1;
            case KEY_END:   f->cursor = f->length; return 1;
            case KEY_BACKSPACE: erase_at(f, f->cursor - 1); return 1;
            case KEY_DC:    erase_at(f, f->cursor); return 1;
            case TUI_KEY_NEWLINE:
                if (!f->multiline) return 0;
                insert(f, L'\n');
                return 1;
            default:        return 0;
        }
    }
    if (ch == 127 || ch == 8) { erase_at(f, f->cursor - 1); return 1; }
    if (ch == 21) { f->length = f->cursor = 0; f->text[0] = L'\0'; return 1; }   /* Ctrl+U clears */
    if (ch < 32) return 0;
    insert(f, (wchar_t)ch);
    return 1;
}

static int char_columns(wchar_t c) {
    int w = wcwidth(c);
    return w < 0 ? 1 : w;
}

void text_field_render(TextField *f, UiRect r, int attr, int focused, TextCaret *caret) {
    if (caret) caret->visible = 0;
    if (r.w <= 0 || r.h <= 0) return;
    tui_fill(r, attr);

    /* Lay the characters out in rows of r.w columns, noting where each row
     * starts and where the cursor lands. */
    int starts[TEXT_FIELD_CAPACITY + 4];
    int rows = 1, col = 0, cursor_row = 0, cursor_col = 0;
    starts[0] = 0;
    for (int i = 0; i <= f->length; i++) {
        if (i < f->length && f->text[i] == L'\n') {         /* a line break ends the row */
            if (i == f->cursor) { cursor_row = rows - 1; cursor_col = col; }
            starts[rows++] = i + 1;
            col = 0;
            continue;
        }
        int w = i < f->length ? char_columns(f->text[i]) : 1;
        if (col + w > r.w && col > 0) { starts[rows++] = i; col = 0; }
        if (i == f->cursor) { cursor_row = rows - 1; cursor_col = col; }
        col += w;
    }
    starts[rows] = f->length;

    if (cursor_row < f->scroll_row) f->scroll_row = cursor_row;
    if (cursor_row >= f->scroll_row + r.h) f->scroll_row = cursor_row - r.h + 1;
    if (f->scroll_row > rows - 1) f->scroll_row = rows - 1 > 0 ? rows - 1 : 0;

    for (int row = 0; row < r.h && f->scroll_row + row < rows; row++) {
        int from = starts[f->scroll_row + row], to = starts[f->scroll_row + row + 1];
        if (to > from && f->text[to - 1] == L'\n') to--;        /* the break itself is not drawn */
        char *line = utf8_from_wide(&f->text[from], (size_t)(to - from));
        if (line) tui_text(r.y + row, r.x, r.w, line, attr);
        free(line);
    }
    if (focused && caret) {
        caret->visible = 1;
        caret->y = r.y + cursor_row - f->scroll_row;
        caret->x = r.x + (cursor_col < r.w ? cursor_col : r.w - 1);
    }
}
