#include "clients/tui/color_pair_cache.h"

#include <ncurses.h>
#include <string.h>

#define FIRST_PAIR 64            /* theme slots use 1..THEME_SLOT_COUNT */
#define MAX_CACHED 4096

typedef struct PairEntry {
    short    fg;
    short    bg;
    unsigned used;
} PairEntry;

static PairEntry s_entries[MAX_CACHED];
static int       s_count = 0;
static unsigned  s_clock = 0;

static int capacity(void) {
    int cap = COLOR_PAIRS - FIRST_PAIR;
    return cap < 0 ? 0 : cap > MAX_CACHED ? MAX_CACHED : cap;
}

int color_pair_cache_available(void) {
    return has_colors() && COLORS >= 256 && capacity() >= 64;
}

int color_pair_cache_get(short fg, short bg) {
    int cap = capacity();
    if (cap <= 0) return 0;
    s_clock++;
    for (int i = 0; i < s_count; i++) {
        if (s_entries[i].fg == fg && s_entries[i].bg == bg) {
            s_entries[i].used = s_clock;
            return FIRST_PAIR + i;
        }
    }
    int slot = s_count;
    if (s_count < cap) {
        s_count++;
    } else {
        slot = 0;
        for (int i = 1; i < s_count; i++) if (s_entries[i].used < s_entries[slot].used) slot = i;
    }
    s_entries[slot] = (PairEntry){ fg, bg, s_clock };
    init_extended_pair(FIRST_PAIR + slot, fg, bg);
    return FIRST_PAIR + slot;
}

void color_pair_cache_reset(void) {
    s_count = 0;
    memset(s_entries, 0, sizeof(s_entries));
}
