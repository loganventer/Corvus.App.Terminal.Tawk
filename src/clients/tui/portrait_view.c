#include "clients/tui/portrait_view.h"
#include "clients/tui/color_pair_cache.h"
#include "utilities/str_util.h"

#include <ctype.h>
#include <ncurses.h>
#include <string.h>
#include <wchar.h>
#include <wctype.h>

/* Badge colours (xterm-256), picked by the JID so a contact keeps its colour. */
static const short BADGE[] = { 25, 30, 31, 61, 64, 94, 96, 97, 130, 132, 133, 136, 166, 168 };

static short badge_colour(const char *jid) {
    unsigned h = 2166136261u;
    for (const char *p = jid; p && *p; p++) h = (h ^ (unsigned char)*p) * 16777619u;
    return BADGE[h % (sizeof(BADGE) / sizeof(BADGE[0]))];
}

static void cell(int y, int x, wchar_t ch, short fg, short bg) {
    int pair = color_pair_cache_get(fg, bg);
    if (!pair) { mvaddch(y, x, ' '); return; }
    wchar_t wstr[2] = { ch, 0 };
    cchar_t c;
    setcchar(&c, wstr, A_BOLD, 0, &pair);
    mvadd_wch(y, x, &c);
}

/* Up to two initials from the first letters of the first two words. */
static int initials(const char *name, wchar_t out[2]) {
    int n = 0;
    mbstate_t st;
    memset(&st, 0, sizeof(st));
    int at_word = 1;
    for (const char *p = name ? name : ""; *p && n < 2;) {
        wchar_t wc;
        size_t len = mbrtowc(&wc, p, strlen(p), &st);
        if (len == (size_t)-1 || len == (size_t)-2 || len == 0) break;
        if (iswalpha((wint_t)wc) || iswdigit((wint_t)wc)) {
            if (at_word) out[n++] = (wchar_t)towupper((wint_t)wc);
            at_word = 0;
        } else if (iswspace((wint_t)wc)) {
            at_word = 1;
        }
        p += len;
    }
    return n;
}

/* The background colour already drawn at (y, x): the row, bar or panel behind the badge. */
static short backdrop(int y, int x) {
    cchar_t c;
    if (mvin_wch(y, x, &c) == ERR) return -1;
    wchar_t wch[CCHARW_MAX + 1];
    attr_t attrs;
    int pair = 0;
    if (getcchar(&c, wch, &attrs, NULL, &pair) == ERR) return -1;
    int fg = -1, bg = -1;
    if (extended_pair_content(pair, &fg, &bg) == ERR) return -1;
    return (short)bg;
}

/* Blanks the box in whatever colour is already behind it; returns that pair. */
static int clear_keeping_backdrop(UiRect r) {
    cchar_t c;
    wchar_t wch[CCHARW_MAX + 1];
    attr_t attrs = 0;
    int pair = 0;
    if (mvin_wch(r.y, r.x, &c) == ERR || getcchar(&c, wch, &attrs, NULL, &pair) == ERR) pair = 0;
    wchar_t blank[2] = { L' ', 0 };
    cchar_t space;
    setcchar(&space, blank, 0, 0, &pair);
    for (int y = 0; y < r.h; y++) for (int x = 0; x < r.w; x++) mvadd_wch(r.y + y, r.x + x, &space);
    return pair;
}

static void badge(UiRect r, const char *jid, const char *name) {
    short behind = backdrop(r.y, r.x);
    short bg = badge_colour(jid);
    for (int y = 0; y < r.h; y++) for (int x = 0; x < r.w; x++) cell(r.y + y, r.x + x, L' ', 231, bg);
    /* Round the corners: quarter blocks in the badge colour on the background. */
    if (r.w >= 3 && r.h >= 2) {
        short base = behind;
        cell(r.y, r.x, L'\x2597', bg, base);                           /* ▗ */
        cell(r.y, r.x + r.w - 1, L'\x2596', bg, base);                 /* ▖ */
        cell(r.y + r.h - 1, r.x, L'\x259D', bg, base);                 /* ▝ */
        cell(r.y + r.h - 1, r.x + r.w - 1, L'\x2598', bg, base);       /* ▘ */
    }
    wchar_t letters[2] = { L'?', 0 };
    int n = initials(name, letters);
    if (n == 0) n = 1;
    int y = r.y + (r.h - 1) / 2, x = r.x + (r.w - n) / 2;
    for (int i = 0; i < n; i++) cell(y, x + i, letters[i], 231, bg);
}

int portrait_draw(UiRect r, const char *jid, const char *name, const char *picture,
                  ThumbnailCache *thumbs, int pixel_images, ImagePlacement *placement) {
    if (r.w < 1 || r.h < 1) return 0;
    int have = picture && picture[0];
    if (have && pixel_images && placement) {
        int behind = clear_keeping_backdrop(r);        /* the circle's corners show the bar or row behind */
        memset(placement, 0, sizeof(*placement));
        placement->message = -1;                      /* a file, not a message picture */
        placement->round = 1;                         /* like WhatsApp; the full view shows the square */
        placement->y = r.y;
        placement->x = r.x;
        placement->cols = r.w;
        placement->rows = r.h;
        placement->attr = behind;
        str_copy(placement->path, sizeof(placement->path), picture);
        str_copy(placement->id, sizeof(placement->id), jid ? jid : "");
        return 1;
    }
    if (have && r.h >= 4) {                           /* big enough for half blocks to show a face */
        const Thumbnail *t = thumbnail_cache_get_path(thumbs, picture, r.w, r.h);
        if (t) {
            thumbnail_draw(t, r.y + (r.h - t->rows) / 2, r.x + (r.w - t->cols) / 2);
            return 0;
        }
    }
    badge(r, jid, name);
    return 0;
}
