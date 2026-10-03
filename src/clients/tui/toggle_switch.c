#include "clients/tui/toggle_switch.h"
#include "clients/tui/tui_draw.h"
#include "clients/tui/tui_palette.h"

const char *toggle_switch_text(int on) {
    return on ? "\xE2\x94\x81\xE2\x94\x81\xE2\x97\x8F"      /* ━━● */
              : "\xE2\x97\x8B\xE2\x94\x80\xE2\x94\x80";     /* ○── */
}

void toggle_switch_draw(int y, int x, int on, int attr) {
    tui_text(y, x, TOGGLE_SWITCH_COLUMNS, toggle_switch_text(on),
             on ? tui_palette_attr(THEME_SLOT_OK) | ATTR_BOLD : attr | ATTR_DIM);
}
