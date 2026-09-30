#include "clients/tui/message_info_panel.h"
#include "clients/tui/tui_draw.h"
#include "clients/tui/tui_palette.h"
#include "utilities/clock_util.h"
#include "utilities/str_util.h"

#include <ncurses.h>
#include <stdio.h>
#include <string.h>

#define WIDTH     60
#define MAX_LINES 256

void message_info_panel_open(MessageInfoPanel *p, const Message *m, int group) {
    memset(p, 0, sizeof(*p));
    str_copy(p->message_id, sizeof(p->message_id), m->id);
    const char *text = m->text && m->text[0] ? m->text : "";
    size_t n = strcspn(text, "\n");
    if (n >= sizeof(p->excerpt)) n = sizeof(p->excerpt) - 1;
    memcpy(p->excerpt, text, n);
    p->excerpt[n] = '\0';
    p->sent_at = m->timestamp;
    p->group = group;
    p->open = 1;
}

PopupResult message_info_panel_key(MessageInfoPanel *p, int is_key, int ch) {
    if ((!is_key && (ch == 27 || ch == 'q' || ch == '\n' || ch == '\r')) || (is_key && ch == KEY_ENTER)) {
        p->open = 0;
        return POPUP_CLOSED;
    }
    if (is_key && ch == KEY_UP && p->scroll > 0) p->scroll--;
    else if (is_key && ch == KEY_DOWN) p->scroll++;
    return POPUP_NONE;
}

PopupResult message_info_panel_click(MessageInfoPanel *p, int y, int x) {
    if (ui_rect_contains(p->last_rect, y, x)) return POPUP_NONE;
    p->open = 0;
    return POPUP_CLOSED;
}

/* The lines to show, as text with a style each. */
typedef struct Line {
    char text[200];
    char right[80];            /* right-aligned: a time */
    int  attr;
} Line;

static void add(Line *lines, int *n, int attr, const char *a, const char *b) {
    if (*n >= MAX_LINES) return;
    snprintf(lines[*n].text, sizeof(lines[*n].text), "%s%s", a, b ? b : "");
    lines[*n].right[0] = '\0';
    lines[(*n)++].attr = attr;
}

static void when(int64_t at, int use_24h, char *out, size_t size) {
    if (at > 0) clock_format_relative(at, use_24h, out, size);
    else str_copy(out, size, "");
}

/* "  Name                  Yesterday 14:02", the time against the right edge. */
static void person(Line *lines, int *n, int attr, const char *name, int64_t at, const char *extra, int use_24h) {
    if (*n >= MAX_LINES) return;
    char t[48];
    when(at, use_24h, t, sizeof(t));
    add(lines, n, attr, "  ", name);
    snprintf(lines[*n - 1].right, sizeof(lines[*n - 1].right), "%s%s", t, extra ? extra : "");
}

void message_info_panel_render(MessageInfoPanel *p, UiRect a, const Receipt *r, int count, int use_24h) {
    static Line lines[MAX_LINES];
    int n = 0;
    int base = tui_palette_attr(THEME_SLOT_BASE), dim = tui_palette_attr(THEME_SLOT_DIM);
    int head = tui_palette_attr(THEME_SLOT_ACCENT) | ATTR_BOLD;
    char t[48], row[200];

    if (p->excerpt[0]) add(lines, &n, dim, "\xE2\x80\x9C", p->excerpt);     /* “ */
    when(p->sent_at, use_24h, t, sizeof(t));
    add(lines, &n, dim, "Sent ", t);
    add(lines, &n, base, "", NULL);

    if (count == 0) {
        add(lines, &n, base, "No receipts yet.", NULL);
        add(lines, &n, dim, "They appear as people receive and read it. People who", NULL);
        add(lines, &n, dim, "turned off read receipts only ever show as delivered.", NULL);
    } else if (!p->group) {
        const Receipt *one = &r[0];
        when(one->delivered_at, use_24h, t, sizeof(t));
        snprintf(row, sizeof(row), "\xE2\x9C\x93\xE2\x9C\x93 Delivered  %s", one->delivered_at ? t : "not yet");
        add(lines, &n, base, row, NULL);
        when(one->read_at, use_24h, t, sizeof(t));
        snprintf(row, sizeof(row), "\xE2\x9C\x93\xE2\x9C\x93 Read       %s", one->read_at ? t : "not yet");
        add(lines, &n, one->read_at ? head : dim, row, NULL);
        if (one->played_at) {
            when(one->played_at, use_24h, t, sizeof(t));
            add(lines, &n, base, "\xE2\x96\xB6  Played     ", t);
        }
    } else {
        int read = 0, delivered = 0;
        for (int i = 0; i < count; i++) { if (r[i].read_at) read++; else if (r[i].delivered_at) delivered++; }
        snprintf(row, sizeof(row), "Read by %d", read);
        add(lines, &n, head, row, NULL);
        for (int i = 0; i < count; i++) {
            if (!r[i].read_at) continue;
            char extra[64] = "";
            if (r[i].played_at) {
                when(r[i].played_at, use_24h, t, sizeof(t));
                snprintf(extra, sizeof(extra), " \xC2\xB7 played %s", t);
            }
            person(lines, &n, base, r[i].name[0] ? r[i].name : r[i].jid, r[i].read_at, extra, use_24h);
        }
        if (!read) add(lines, &n, dim, "  no one yet", NULL);
        add(lines, &n, base, "", NULL);
        snprintf(row, sizeof(row), "Delivered to %d", delivered);
        add(lines, &n, head, row, NULL);
        for (int i = 0; i < count; i++) {
            if (r[i].read_at || !r[i].delivered_at) continue;
            person(lines, &n, base, r[i].name[0] ? r[i].name : r[i].jid, r[i].delivered_at, NULL, use_24h);
        }
        if (!delivered) add(lines, &n, dim, "  no one waiting", NULL);
    }

    int w = a.w < WIDTH ? a.w : WIDTH;
    int h = n + 4;
    if (h > a.h) h = a.h;
    UiRect box = { a.y + (a.h - h) / 2, a.x + (a.w - w) / 2, h, w };
    p->last_rect = box;
    tui_box(box, "\xE2\x84\xB9 Message info", tui_palette_attr(THEME_SLOT_BORDER));
    tui_fill((UiRect){ box.y + 1, box.x + 1, box.h - 2, box.w - 2 }, base);
    int rows = box.h - 3;
    int max_scroll = n > rows ? n - rows : 0;
    if (p->scroll > max_scroll) p->scroll = max_scroll;
    for (int i = 0; i < rows && p->scroll + i < n; i++) {
        const Line *l = &lines[p->scroll + i];
        int right = l->right[0] ? tui_text_right(box.y + 1 + i, box.x + box.w - 2, box.w / 2, l->right, l->attr) + 1 : 0;
        tui_text(box.y + 1 + i, box.x + 2, box.w - 4 - right, l->text, l->attr);
    }
    tui_text_center(box.y + box.h - 2, box.x, box.w,
                    max_scroll ? "\xE2\x86\x91\xE2\x86\x93 scroll \xC2\xB7 Esc close" : "Esc close", dim);
}
