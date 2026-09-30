#include "clients/tui/text_reader.h"
#include "clients/tui/styled_text_view.h"
#include "clients/tui/tui_draw.h"
#include "clients/tui/tui_palette.h"
#include "utilities/str_util.h"
#include "utilities/utf8_text.h"

#include <ncurses.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void text_reader_open(TextReader *r, const char *title, const char *text) {
    text_reader_close(r);
    r->text = str_dup(text ? text : "");
    str_copy(r->title, sizeof(r->title), title ? title : "");
    r->scroll = 0;
    r->open = r->text != NULL;
}

void text_reader_open_styled(TextReader *r, const char *title, const StyledText *styled) {
    text_reader_open(r, title, styled->text);
    if (!r->open) return;
    r->styled.runs = malloc(sizeof(StyledRun) * (size_t)(styled->run_count ? styled->run_count : 1));
    r->styled.text = r->styled.runs ? str_dup(styled->text) : NULL;
    if (!r->styled.text) { styled_text_dispose(&r->styled); return; }
    memcpy(r->styled.runs, styled->runs, sizeof(StyledRun) * (size_t)styled->run_count);
    r->styled.run_count = styled->run_count;
}

void text_reader_close(TextReader *r) {
    free(r->text);
    r->text = NULL;
    styled_text_dispose(&r->styled);
    r->open = 0;
}

static void clamp(TextReader *r) {
    int max = r->total_rows - r->page_rows;
    if (r->scroll > max) r->scroll = max;
    if (r->scroll < 0) r->scroll = 0;
}

PopupResult text_reader_key(TextReader *r, int is_key, int ch) {
    if (!is_key && (ch == 27 || ch == 'q' || ch == '\n' || ch == '\r')) { text_reader_close(r); return POPUP_CLOSED; }
    if (!is_key) {
        if (ch == 'j' || ch == ' ') r->scroll += ch == ' ' ? r->page_rows - 1 : 1;
        else if (ch == 'k') r->scroll--;
    } else if (ch == KEY_DOWN) r->scroll++;
    else if (ch == KEY_UP) r->scroll--;
    else if (ch == KEY_NPAGE) r->scroll += r->page_rows - 1;
    else if (ch == KEY_PPAGE) r->scroll -= r->page_rows - 1;
    else if (ch == KEY_HOME) r->scroll = 0;
    else if (ch == KEY_END) r->scroll = r->total_rows;
    clamp(r);
    return POPUP_NONE;
}

void text_reader_wheel(TextReader *r, int delta) {
    r->scroll += delta * 3;
    clamp(r);
}

void text_reader_render(TextReader *r, UiRect a) {
    int base = tui_palette_attr(THEME_SLOT_BASE);
    tui_box(a, r->title, tui_palette_attr(THEME_SLOT_BORDER));
    tui_fill((UiRect){ a.y + 1, a.x + 1, a.h - 2, a.w - 2 }, base);
    UiRect body = { a.y + 1, a.x + 2, a.h - 3, a.w - 4 };
    TextLine *lines = NULL;
    int n = utf8_wrap(r->text, body.w, &lines);
    r->total_rows = n;
    r->page_rows = body.h;
    clamp(r);
    for (int k = 0; k < body.h && r->scroll + k < n; k++) {
        const TextLine *l = &lines[r->scroll + k];
        if (r->styled.text) styled_text_view_draw(body.y + k, body.x, body.w, &r->styled, l->offset, l->length, base, tui_palette_attr(THEME_SLOT_ACCENT));
        else tui_text_n(body.y + k, body.x, body.w, r->text + l->offset, l->length, base);
    }
    free(lines);
    char footer[96];
    snprintf(footer, sizeof(footer), "line %d of %d \xC2\xB7 \xE2\x86\x91\xE2\x86\x93 PgUp PgDn scroll \xC2\xB7 Esc close",
             n ? r->scroll + 1 : 0, n);
    tui_text_center(a.y + a.h - 2, a.x, a.w, footer, tui_palette_attr(THEME_SLOT_DIM));
}
