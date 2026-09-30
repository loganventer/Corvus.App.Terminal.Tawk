#include "clients/tui/status_colour.h"
#include "clients/tui/color_pair_cache.h"
#include "clients/tui/tui_palette.h"
#include "utilities/color_util.h"

#include <ncurses.h>

int status_colour_attr(uint32_t argb) {
    if (!argb || !color_pair_cache_available()) return 0;
    short bg = color_rgb_to_xterm256((int)((argb >> 16) & 0xFF), (int)((argb >> 8) & 0xFF), (int)(argb & 0xFF));
    int pair = color_pair_cache_get(15, bg);
    return pair > 0 && pair < 256 ? (int)COLOR_PAIR(pair) | ATTR_BOLD : 0;   /* COLOR_PAIR holds 8 bits */
}
