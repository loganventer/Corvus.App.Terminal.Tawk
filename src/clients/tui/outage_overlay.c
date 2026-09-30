#include "clients/tui/outage_overlay.h"
#include "clients/tui/tui_draw.h"
#include "clients/tui/tui_palette.h"
#include "utilities/utf8_text.h"

#include <ncurses.h>
#include <stdio.h>
#include <stdlib.h>

static void format_wait(int64_t ms, char *out, size_t size) {
    int64_t s = (ms + 999) / 1000;
    if (s >= 60) snprintf(out, size, "%lld:%02lld", (long long)(s / 60), (long long)(s % 60));
    else snprintf(out, size, "%llds", (long long)s);
}

void outage_overlay_render(UiRect a, const ConnectionHealth *h) {
    int w = a.w - 8 < 64 ? a.w - 8 : 64;
    int hgt = 11;
    if (w < 24 || a.h < hgt) { w = a.w; hgt = a.h; }
    UiRect box = { a.y + (a.h - hgt) / 2, a.x + (a.w - w) / 2, hgt, w };
    int attr = tui_palette_attr(THEME_SLOT_SIDEBAR_SELECTED);
    tui_box(box, "\xE2\x9A\xA0 WhatsApp is unavailable", attr);

    int inner = w - 4, y = box.y + 2;
    tui_text_center(y++, box.x, box.w, h->title, attr | ATTR_BOLD);
    y++;
    if (h->detail[0]) {
        TextLine *lines = NULL;
        int n = utf8_wrap(h->detail, inner, &lines);
        for (int i = 0; i < n && i < 2; i++) {
            char line[256];
            snprintf(line, sizeof(line), "%.*s", (int)lines[i].length, h->detail + lines[i].offset);
            tui_text_center(y++, box.x, box.w, line, attr);
        }
        free(lines);
    }
    y = box.y + hgt - 4;
    char wait[32], status[160];
    format_wait(h->retry_in_ms, wait, sizeof(wait));
    if (h->breaker == CIRCUIT_OPEN) {
        snprintf(status, sizeof(status), "Paused after %d failed attempts. Trying again in %s.", h->attempt, wait);
    } else if (h->will_retry) {
        snprintf(status, sizeof(status), "Reconnecting in %s (attempt %d)\xE2\x80\xA6", wait, h->attempt + 1);
    } else {
        snprintf(status, sizeof(status), "Press R to reconnect when you are ready.");
    }
    tui_text_center(y, box.x, box.w, status, tui_palette_attr(THEME_SLOT_WARN) | ATTR_BOLD);
    tui_text_center(box.y + hgt - 2, box.x, box.w, "R  retry now      Q  quit tawk", attr | ATTR_DIM);
}
