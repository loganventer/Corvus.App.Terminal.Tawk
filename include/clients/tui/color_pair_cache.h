#ifndef APP_CLIENTS_TUI_COLOR_PAIR_CACHE_H
#define APP_CLIENTS_TUI_COLOR_PAIR_CACHE_H

/* Hands out curses colour pairs for arbitrary (fg, bg) combinations, as
 * picture previews need far more than the theme's fixed pairs. Pairs are
 * taken from a range above the theme's and recycled least recently used. */
int  color_pair_cache_available(void);
/* Pair number for the combination, or 0 when none can be allocated. */
int  color_pair_cache_get(short fg, short bg);
void color_pair_cache_reset(void);

#endif
