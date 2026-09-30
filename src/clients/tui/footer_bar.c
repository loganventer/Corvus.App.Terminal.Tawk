#include "clients/tui/footer_bar.h"
#include "clients/tui/tui_draw.h"
#include "clients/tui/tui_palette.h"

#include <ncurses.h>

/* Toasts on the left, short key hints on the right. */
void footer_bar_render(UiRect r, const char *hints, const char *toast, int is_error) {
    tui_fill(r, tui_palette_attr(THEME_SLOT_BASE));
    int used = 0;
    if (toast && *toast) {
        used = tui_text(r.y, r.x + 1, r.w - 2, toast,
                        tui_palette_attr(is_error ? THEME_SLOT_WARN : THEME_SLOT_ACCENT) | ATTR_BOLD) + 3;
    }
    if (hints && *hints && r.w - used > 10) {
        tui_text_right(r.y, r.x + r.w - 1, r.w - used - 2, hints, tui_palette_attr(THEME_SLOT_DIM));
    }
}
