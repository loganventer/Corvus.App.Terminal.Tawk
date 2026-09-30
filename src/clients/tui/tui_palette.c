#include "clients/tui/tui_palette.h"
#include "utilities/color_util.h"

#include <ncurses.h>

static int s_has_color = 0;

void tui_palette_init(void) {
    s_has_color = has_colors();
    if (!s_has_color) return;
    start_color();
    use_default_colors();
}

static short fit(short c) {
    if (c < 0) return -1;
    return COLORS >= 256 ? c : color_xterm256_to_8(c);
}

#define CONVERSATION_BASE 32   /* conversation pairs: 33 .. 32 + THEME_SLOT_COUNT */

#define EDGE_PAIR_ME   (CONVERSATION_BASE + THEME_SLOT_COUNT + 1)
#define EDGE_PAIR_THEM (CONVERSATION_BASE + THEME_SLOT_COUNT + 2)
#define ACCENT_PAIR_ME   (CONVERSATION_BASE + THEME_SLOT_COUNT + 3)
#define ACCENT_PAIR_THEM (CONVERSATION_BASE + THEME_SLOT_COUNT + 4)

void tui_palette_apply_conversation(const Theme *theme) {
    if (!s_has_color || !theme) return;
    for (int s = 0; s < THEME_SLOT_COUNT; s++) {
        init_pair((short)(CONVERSATION_BASE + s + 1), fit(theme->colors[s].fg), fit(theme->colors[s].bg));
    }
    short chat_bg = fit(theme->colors[THEME_SLOT_CHAT].bg);
    init_pair(EDGE_PAIR_ME, fit(theme->colors[THEME_SLOT_BUBBLE_ME].bg), chat_bg);
    init_pair(EDGE_PAIR_THEM, fit(theme->colors[THEME_SLOT_BUBBLE_THEM].bg), chat_bg);
    short accent = fit(theme->colors[THEME_SLOT_ACCENT].fg);
    init_pair(ACCENT_PAIR_ME, accent, fit(theme->colors[THEME_SLOT_BUBBLE_ME].bg));
    init_pair(ACCENT_PAIR_THEM, accent, fit(theme->colors[THEME_SLOT_BUBBLE_THEM].bg));
}

int tui_palette_bubble_accent_attr(int from_me) {
    if (!s_has_color) return A_BOLD;
    return COLOR_PAIR(from_me ? ACCENT_PAIR_ME : ACCENT_PAIR_THEM);
}

int tui_palette_bubble_edge_attr(int from_me) {
    if (!s_has_color) return A_NORMAL;
    return COLOR_PAIR(from_me ? EDGE_PAIR_ME : EDGE_PAIR_THEM);
}

void tui_palette_apply(const Theme *theme) {
    if (!s_has_color || !theme) return;
    for (int s = 0; s < THEME_SLOT_COUNT; s++) {
        init_pair((short)(s + 1), fit(theme->colors[s].fg), fit(theme->colors[s].bg));
    }
    tui_palette_apply_conversation(theme);
}

int tui_palette_conversation_attr(ThemeSlot slot) {
    if (!s_has_color) return tui_palette_attr(slot);
    return COLOR_PAIR(CONVERSATION_BASE + slot + 1);
}

int tui_palette_attr(ThemeSlot slot) {
    if (!s_has_color) {
        return (slot == THEME_SLOT_SIDEBAR_SELECTED || slot == THEME_SLOT_HEADER ||
                slot == THEME_SLOT_BUBBLE_ME || slot == THEME_SLOT_BLINK) ? A_REVERSE : A_NORMAL;
    }
    return COLOR_PAIR(slot + 1);
}
