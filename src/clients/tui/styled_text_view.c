#include "clients/tui/styled_text_view.h"
#include "clients/tui/tui_draw.h"
#include "clients/tui/tui_palette.h"
#include "core/text_style.h"

#include <ncurses.h>
#include <term.h>

/* Italic where the terminal has it (terminfo "sitm"); underline elsewhere. */
static int italic_attr(void) {
    static int attr = -1;
    if (attr < 0) {
        const char *sitm = tigetstr("sitm");
        attr = sitm && sitm != (char *)-1 ? (int)A_ITALIC : ATTR_UNDERLINE;
    }
    return attr;
}

static int attr_for(int style, int base, int accent) {
    int attr = (style & (TEXT_STYLE_CODE | TEXT_STYLE_BLOCK | TEXT_STYLE_MENTION)) ? accent : base;
    if (style & (TEXT_STYLE_BOLD | TEXT_STYLE_MENTION)) attr |= ATTR_BOLD;
    if (style & TEXT_STYLE_ITALIC) attr |= italic_attr();
    if (style & (TEXT_STYLE_STRIKE | TEXT_STYLE_QUOTE)) attr |= ATTR_DIM;
    return attr;
}

int styled_text_view_draw(int y, int x, int max_cols, const StyledText *t, size_t from, size_t length, int base, int accent) {
    size_t to = from + length;
    int used = 0;
    for (int i = 0; i < t->run_count && used < max_cols; i++) {
        const StyledRun *run = &t->runs[i];
        if (run->end <= from || run->start >= to) continue;
        size_t a = run->start > from ? run->start : from;
        size_t b = run->end < to ? run->end : to;
        used += tui_text_n(y, x + used, max_cols - used, t->text + a, b - a, attr_for(run->style, base, accent));
    }
    return used;
}
