#include "clients/tui/splash_view.h"
#include "clients/tui/color_pair_cache.h"
#include "clients/tui/tui_draw.h"
#include "clients/tui/tui_palette.h"
#include "utilities/utf8_text.h"

#include <ncurses.h>
#include <stdlib.h>
#include <string.h>
#include <wchar.h>

#define DURATION_MS 2200
#define WIPE_MS     700      /* the logo drawing itself in */
#define TYPE_MS     900      /* the tagline typing out, after the wipe */
#define FADE_MS     350      /* dimming at the end */
#define SHINE_MS    1400     /* one sweep of the shine */

/* "TAWK" in the ANSI Shadow figlet font. */
static const char *const LOGO[] = {
    "\xE2\x96\x88\xE2\x96\x88\xE2\x96\x88\xE2\x96\x88\xE2\x96\x88\xE2\x96\x88\xE2\x96\x88\xE2\x96\x88\xE2\x95\x97 \xE2\x96\x88\xE2\x96\x88\xE2\x96\x88\xE2\x96\x88\xE2\x96\x88\xE2\x95\x97 \xE2\x96\x88\xE2\x96\x88\xE2\x95\x97    \xE2\x96\x88\xE2\x96\x88\xE2\x95\x97\xE2\x96\x88\xE2\x96\x88\xE2\x95\x97  \xE2\x96\x88\xE2\x96\x88\xE2\x95\x97",
    "\xE2\x95\x9A\xE2\x95\x90\xE2\x95\x90\xE2\x96\x88\xE2\x96\x88\xE2\x95\x94\xE2\x95\x90\xE2\x95\x90\xE2\x95\x9D\xE2\x96\x88\xE2\x96\x88\xE2\x95\x94\xE2\x95\x90\xE2\x95\x90\xE2\x96\x88\xE2\x96\x88\xE2\x95\x97\xE2\x96\x88\xE2\x96\x88\xE2\x95\x91    \xE2\x96\x88\xE2\x96\x88\xE2\x95\x91\xE2\x96\x88\xE2\x96\x88\xE2\x95\x91 \xE2\x96\x88\xE2\x96\x88\xE2\x95\x94\xE2\x95\x9D",
    "   \xE2\x96\x88\xE2\x96\x88\xE2\x95\x91   \xE2\x96\x88\xE2\x96\x88\xE2\x96\x88\xE2\x96\x88\xE2\x96\x88\xE2\x96\x88\xE2\x96\x88\xE2\x95\x91\xE2\x96\x88\xE2\x96\x88\xE2\x95\x91 \xE2\x96\x88\xE2\x95\x97 \xE2\x96\x88\xE2\x96\x88\xE2\x95\x91\xE2\x96\x88\xE2\x96\x88\xE2\x96\x88\xE2\x96\x88\xE2\x96\x88\xE2\x95\x94\xE2\x95\x9D ",
    "   \xE2\x96\x88\xE2\x96\x88\xE2\x95\x91   \xE2\x96\x88\xE2\x96\x88\xE2\x95\x94\xE2\x95\x90\xE2\x95\x90\xE2\x96\x88\xE2\x96\x88\xE2\x95\x91\xE2\x96\x88\xE2\x96\x88\xE2\x95\x91\xE2\x96\x88\xE2\x96\x88\xE2\x96\x88\xE2\x95\x97\xE2\x96\x88\xE2\x96\x88\xE2\x95\x91\xE2\x96\x88\xE2\x96\x88\xE2\x95\x94\xE2\x95\x90\xE2\x96\x88\xE2\x96\x88\xE2\x95\x97 ",
    "   \xE2\x96\x88\xE2\x96\x88\xE2\x95\x91   \xE2\x96\x88\xE2\x96\x88\xE2\x95\x91  \xE2\x96\x88\xE2\x96\x88\xE2\x95\x91\xE2\x95\x9A\xE2\x96\x88\xE2\x96\x88\xE2\x96\x88\xE2\x95\x94\xE2\x96\x88\xE2\x96\x88\xE2\x96\x88\xE2\x95\x94\xE2\x95\x9D\xE2\x96\x88\xE2\x96\x88\xE2\x95\x91  \xE2\x96\x88\xE2\x96\x88\xE2\x95\x97",
    "   \xE2\x95\x9A\xE2\x95\x90\xE2\x95\x9D   \xE2\x95\x9A\xE2\x95\x90\xE2\x95\x9D  \xE2\x95\x9A\xE2\x95\x90\xE2\x95\x9D \xE2\x95\x9A\xE2\x95\x90\xE2\x95\x90\xE2\x95\x9D\xE2\x95\x9A\xE2\x95\x90\xE2\x95\x90\xE2\x95\x9D \xE2\x95\x9A\xE2\x95\x90\xE2\x95\x9D  \xE2\x95\x9A\xE2\x95\x90\xE2\x95\x9D",
};
#define LOGO_ROWS ((int)(sizeof(LOGO) / sizeof(LOGO[0])))

static const char TAGLINE[] = "WhatsApp in your terminal";
static const char ACRONYM[] = "Terminal Access to WhatsApp Konnector";   /* what the name stands for */

/* WhatsApp greens, dark to bright (xterm-256), for the shine and the fade. */
static const short GREENS[] = { 22, 28, 34, 35, 41, 47, 84, 121, 157, 195 };
#define GREEN_COUNT ((int)(sizeof(GREENS) / sizeof(GREENS[0])))

void splash_view_start(SplashView *v, int64_t now) {
    v->active = 1;
    v->started_ms = now;
    v->ends_ms = now + DURATION_MS;
}

void splash_view_skip(SplashView *v) { v->active = 0; }

int splash_view_active(SplashView *v, int64_t now) {
    if (v->active && now >= v->ends_ms) v->active = 0;
    return v->active;
}

/* The theme's background colour, so the logo sits on the same colour as the rest. */
static short theme_background(void) {
    int fg = 0, bg = 0;
    if (extended_pair_content(PAIR_NUMBER(tui_palette_attr(THEME_SLOT_BASE)), &fg, &bg) != OK) return COLOR_BLACK;
    return (short)bg;
}

/* One cell in an exact xterm-256 colour on the theme's background, falling
 * back to the theme's accent when the terminal has too few colours. */
static void put(int y, int x, const wchar_t *ch, short colour, int bold) {
    int pair = color_pair_cache_available() ? color_pair_cache_get(colour, theme_background()) : 0;
    cchar_t c;
    if (pair) {
        setcchar(&c, ch, bold ? A_BOLD : A_NORMAL, 0, &pair);
    } else {
        short theme = (short)PAIR_NUMBER(tui_palette_attr(THEME_SLOT_ACCENT));
        setcchar(&c, ch, bold ? A_BOLD : A_NORMAL, theme, NULL);
    }
    mvadd_wch(y, x, &c);
}

/* How bright column `col` of `width` is at `t` ms: a soft band sweeping
 * left to right, brightest in the middle, over a mid-green base. */
static int shade(int col, int width, int64_t t, int fade) {
    int64_t phase = t % SHINE_MS;
    int centre = (int)((phase * (width + 24)) / SHINE_MS) - 12;
    int d = col - centre;
    if (d < 0) d = -d;
    int level = 4 + (d < 8 ? (8 - d) * 5 / 8 : 0);
    level -= fade;
    if (level < 0) level = 0;
    if (level >= GREEN_COUNT) level = GREEN_COUNT - 1;
    return level;
}

static void draw_logo(int top, int left, int64_t t, int fade) {
    int width = utf8_columns(LOGO[0]);
    int revealed = t >= WIPE_MS ? width : (int)((t * width) / WIPE_MS);
    for (int row = 0; row < LOGO_ROWS; row++) {
        wchar_t line[128];
        size_t n = mbstowcs(line, LOGO[row], 127);
        if (n == (size_t)-1) continue;
        for (size_t col = 0; col < n && (int)col < revealed; col++) {
            if (line[col] == L' ') continue;
            wchar_t ch[2] = { line[col], 0 };
            int edge = t < WIPE_MS && (int)col >= revealed - 2;    /* the bright leading edge */
            int level = edge ? GREEN_COUNT - 1 : shade((int)col, width, t, fade);
            put(top + row, left + (int)col, ch, GREENS[level], level > 5);
        }
    }
}

/* Three dots that rise and fall one after another, like someone typing. */
static void draw_dots(int y, int centre_x, int64_t t, int fade) {
    static const wchar_t *const FRAMES[] = { L"\x2219", L"\x2022", L"\x25CF" };   /* ∙ • ● */
    for (int i = 0; i < 3; i++) {
        int64_t p = (t / 120 + (3 - i) * 2) % 6;
        int f = p < 3 ? (int)p : (int)(5 - p);
        int level = 5 + f * 2 - fade;
        if (level < 0) level = 0;
        put(y, centre_x - 2 + i * 2, FRAMES[f], GREENS[level < GREEN_COUNT ? level : GREEN_COUNT - 1], 1);
    }
}

void splash_view_render(const SplashView *v, UiRect s, int64_t now, const char *detail, const char *status) {
    int64_t t = now - v->started_ms;
    int64_t left_ms = v->ends_ms - now;
    int fade = left_ms < FADE_MS ? (int)((FADE_MS - left_ms) * 6 / FADE_MS) : 0;
    int base = tui_palette_attr(THEME_SLOT_BASE);
    tui_fill(s, base);

    int width = utf8_columns(LOGO[0]);
    int block_h = LOGO_ROWS + 7;
    int top = s.y + (s.h - block_h) / 2;
    if (top < s.y) top = s.y;
    if (width + 2 <= s.w) {
        draw_logo(top, s.x + (s.w - width) / 2, t, fade);
    } else {
        tui_text_center(top + LOGO_ROWS / 2, s.x, s.w, "tawk", tui_palette_attr(THEME_SLOT_ACCENT) | ATTR_BOLD);
    }

    tui_text_center(top + LOGO_ROWS, s.x, s.w, ACRONYM, base);

    /* The tagline types itself out once the logo is in. */
    int typed = t <= WIPE_MS ? 0 : (int)(((t - WIPE_MS) * (int64_t)strlen(TAGLINE)) / TYPE_MS);
    if (typed > (int)strlen(TAGLINE)) typed = (int)strlen(TAGLINE);
    int tag_y = top + LOGO_ROWS + 2;
    int tag_x = s.x + (s.w - (int)strlen(TAGLINE)) / 2;
    tui_text_n(tag_y, tag_x, s.w, TAGLINE, (unsigned long)typed, base | ATTR_BOLD);
    if (typed < (int)strlen(TAGLINE) && t > WIPE_MS && (t / 250) % 2 == 0) tui_text(tag_y, tag_x + typed, 1, "\xE2\x96\x8F", base);   /* ▏ caret */

    draw_dots(tag_y + 2, s.x + s.w / 2, t, fade);
    if (status && *status) tui_text_center(tag_y + 3, s.x, s.w, status, base | ATTR_DIM);
    if (detail && *detail) tui_text_center(s.y + s.h - 2, s.x, s.w, detail, tui_palette_attr(THEME_SLOT_DIM));
}
