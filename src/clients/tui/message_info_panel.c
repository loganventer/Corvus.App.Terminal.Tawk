#include "clients/tui/message_info_panel.h"
#include "clients/tui/tui_draw.h"
#include "clients/tui/tui_palette.h"
#include "engines/played_time.h"
#include "engines/receipt_elapsed_text.h"
#include "utilities/clock_util.h"
#include "utilities/str_util.h"

#include <ncurses.h>
#include <stdio.h>
#include <string.h>

#define WIDTH       60
#define MAX_LINES   256
#define ARROW_SHUT  "\xE2\x96\xB8 "        /* ▸ */
#define ARROW_OPEN  "\xE2\x96\xBE "        /* ▾ */
#define SUB_INDENT  "      "

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
    p->playable = m->from_me && played_time_applies(m->type);
    p->open = 1;
}

static void toggle(MessageInfoPanel *p, int item, int want) {
    if (item < 0 || item >= MESSAGE_INFO_MAX_ITEMS) return;
    uint64_t bit = (uint64_t)1 << item;
    if (want < 0) p->opened ^= bit;
    else if (want) p->opened |= bit;
    else p->opened &= ~bit;
}

PopupResult message_info_panel_key(MessageInfoPanel *p, int is_key, int ch) {
    int enter = (!is_key && (ch == '\n' || ch == '\r')) || (is_key && ch == KEY_ENTER);
    if ((!is_key && (ch == 27 || ch == 'q')) || (enter && p->item_count == 0)) {
        p->open = 0;
        return POPUP_CLOSED;
    }
    if (is_key && ch == KEY_UP) {
        if (p->item_count > 0) { if (p->cursor > 0) p->cursor--; }
        else if (p->scroll > 0) p->scroll--;
    } else if (is_key && ch == KEY_DOWN) {
        if (p->item_count > 0) { if (p->cursor < p->item_count - 1) p->cursor++; }
        else p->scroll++;
    } else if (enter || (!is_key && ch == ' ')) {
        toggle(p, p->cursor, -1);
    } else if (is_key && ch == KEY_RIGHT) {
        toggle(p, p->cursor, 1);
    } else if (is_key && ch == KEY_LEFT) {
        toggle(p, p->cursor, 0);
    }
    return POPUP_NONE;
}

PopupResult message_info_panel_click(MessageInfoPanel *p, int y, int x) {
    if (!ui_rect_contains(p->last_rect, y, x)) {
        p->open = 0;
        return POPUP_CLOSED;
    }
    int row = y - (p->last_rect.y + 1);
    if (row >= 0 && row < MESSAGE_INFO_MAX_ROWS && p->row_item[row] >= 0) {
        p->cursor = p->row_item[row];
        toggle(p, p->cursor, -1);
    }
    return POPUP_NONE;
}

/* The lines to show, as text with a style each; `item` is the number of a
 * line that opens, or -1. */
typedef struct Line {
    char text[200];
    char right[80];            /* right-aligned: a time */
    int  attr;
    int  item;
} Line;

static void add(Line *lines, int *n, int attr, const char *a, const char *b) {
    if (*n >= MAX_LINES) return;
    snprintf(lines[*n].text, sizeof(lines[*n].text), "%s%s", a, b ? b : "");
    lines[*n].right[0] = '\0';
    lines[*n].item = -1;
    lines[(*n)++].attr = attr;
}

static void when(int64_t at, int use_24h, char *out, size_t size) {
    if (at > 0) clock_format_relative(at, use_24h, out, size);
    else str_copy(out, size, "");
}

static int is_open(const MessageInfoPanel *p, int item) {
    return item >= 0 && item < MESSAGE_INFO_MAX_ITEMS && (int)((p->opened >> item) & 1u);
}

/* One line with a time: "▸ ✓✓ Read" and the time against the right edge. Its
 * arrow opens it to the exact date and time and how long after the message
 * it came. A line with no time yet has no arrow. */
static void timed(Line *lines, int *n, int *items, const MessageInfoPanel *p, int attr, const char *indent,
                  const char *label, int64_t at, int use_24h, int show_after) {
    if (*n >= MAX_LINES) return;
    int item = at > 0 ? (*items)++ : -1;
    int open = is_open(p, item);
    char t[48];
    when(at, use_24h, t, sizeof(t));
    snprintf(lines[*n].text, sizeof(lines[*n].text), "%s%s%s", indent, item < 0 ? "  " : open ? ARROW_OPEN : ARROW_SHUT, label);
    snprintf(lines[*n].right, sizeof(lines[*n].right), "%s", at > 0 ? t : "not yet");
    lines[*n].item = item;
    lines[(*n)++].attr = attr;
    if (!open) return;
    int dim = tui_palette_attr(THEME_SLOT_DIM);
    char exact[40], after[96];
    clock_format_exact(at, exact, sizeof(exact));
    add(lines, n, dim, SUB_INDENT, exact);
    if (show_after) {
        receipt_elapsed_text(p->sent_at, at, after, sizeof(after));
        if (after[0]) add(lines, n, dim, SUB_INDENT, after);
    }
}

/* One person of a group: their furthest time on the right, and, opened, each step they have reached. */
static void person(Line *lines, int *n, int *items, const MessageInfoPanel *p, int attr, const char *name, const Receipt *r,
                   int64_t at, const char *extra, int use_24h) {
    if (*n >= MAX_LINES) return;
    int item = (*items)++;
    int open = is_open(p, item);
    char t[48];
    when(at, use_24h, t, sizeof(t));
    snprintf(lines[*n].text, sizeof(lines[*n].text), "  %s%s", open ? ARROW_OPEN : ARROW_SHUT, name);
    snprintf(lines[*n].right, sizeof(lines[*n].right), "%s%s", t, extra ? extra : "");
    lines[*n].item = item;
    lines[(*n)++].attr = attr;
    if (!open) return;
    int dim = tui_palette_attr(THEME_SLOT_DIM);
    char exact[40], row[120];
    const char *labels[3] = { "Delivered ", "Read ", "Played " };
    int64_t times[3] = { r->delivered_at, r->read_at, r->played_at };
    for (int i = 0; i < 3; i++) {
        if (times[i] <= 0) continue;
        clock_format_exact(times[i], exact, sizeof(exact));
        snprintf(row, sizeof(row), "%s%s", labels[i], exact);
        add(lines, n, dim, SUB_INDENT, row);
    }
}

void message_info_panel_render(MessageInfoPanel *p, UiRect a, const Receipt *r, int count, int use_24h) {
    static Line lines[MAX_LINES];
    int n = 0, items = 0;
    int base = tui_palette_attr(THEME_SLOT_BASE), dim = tui_palette_attr(THEME_SLOT_DIM);
    int head = tui_palette_attr(THEME_SLOT_ACCENT) | ATTR_BOLD;
    char row[200];

    if (p->excerpt[0]) add(lines, &n, dim, "\xE2\x80\x9C", p->excerpt);     /* “ */
    timed(lines, &n, &items, p, dim, "", "Sent", p->sent_at, use_24h, 0);
    add(lines, &n, base, "", NULL);

    if (count == 0) {
        add(lines, &n, base, "No receipts yet.", NULL);
        add(lines, &n, dim, "They appear as people receive and read it. People who", NULL);
        add(lines, &n, dim, "turned off read receipts only ever show as delivered.", NULL);
    } else if (!p->group) {
        const Receipt *one = &r[0];
        timed(lines, &n, &items, p, base, "", "\xE2\x9C\x93\xE2\x9C\x93 Delivered", one->delivered_at, use_24h, 1);
        timed(lines, &n, &items, p, one->read_at ? head : dim, "", "\xE2\x9C\x93\xE2\x9C\x93 Read", one->read_at, use_24h, 1);
        if (p->playable || one->played_at) {
            timed(lines, &n, &items, p, one->played_at ? base : dim, "", "\xE2\x96\xB6 Played", one->played_at, use_24h, 1);
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
                char t[48];
                when(r[i].played_at, use_24h, t, sizeof(t));
                snprintf(extra, sizeof(extra), " \xC2\xB7 played %s", t);
            }
            person(lines, &n, &items, p, base, r[i].name[0] ? r[i].name : r[i].jid, &r[i], r[i].read_at, extra, use_24h);
        }
        if (!read) add(lines, &n, dim, "  no one yet", NULL);
        add(lines, &n, base, "", NULL);
        snprintf(row, sizeof(row), "Delivered to %d", delivered);
        add(lines, &n, head, row, NULL);
        for (int i = 0; i < count; i++) {
            if (r[i].read_at || !r[i].delivered_at) continue;
            person(lines, &n, &items, p, base, r[i].name[0] ? r[i].name : r[i].jid, &r[i], r[i].delivered_at, NULL, use_24h);
        }
        if (!delivered) add(lines, &n, dim, "  no one waiting", NULL);
    }

    p->item_count = items;
    if (p->cursor >= items) p->cursor = items > 0 ? items - 1 : 0;

    int w = a.w < WIDTH ? a.w : WIDTH;
    int h = n + 4;
    if (h > a.h) h = a.h;
    UiRect box = { a.y + (a.h - h) / 2, a.x + (a.w - w) / 2, h, w };
    p->last_rect = box;
    int rows = box.h - 3;
    int max_scroll = n > rows ? n - rows : 0;
    /* The line the arrow keys are on stays in view. */
    for (int i = 0; i < n && items > 0; i++) {
        if (lines[i].item != p->cursor) continue;
        if (i < p->scroll) p->scroll = i;
        else if (rows > 0 && i >= p->scroll + rows) p->scroll = i - rows + 1;
        break;
    }
    if (p->scroll > max_scroll) p->scroll = max_scroll;
    if (p->scroll < 0) p->scroll = 0;

    tui_box(box, "\xE2\x84\xB9 Message info", tui_palette_attr(THEME_SLOT_BORDER));
    tui_fill((UiRect){ box.y + 1, box.x + 1, box.h - 2, box.w - 2 }, base);
    for (int i = 0; i < MESSAGE_INFO_MAX_ROWS; i++) p->row_item[i] = -1;
    for (int i = 0; i < rows && p->scroll + i < n; i++) {
        const Line *l = &lines[p->scroll + i];
        int attr = l->item >= 0 && l->item == p->cursor ? head : l->attr;
        if (i < MESSAGE_INFO_MAX_ROWS) p->row_item[i] = (short)l->item;
        int right = l->right[0] ? tui_text_right(box.y + 1 + i, box.x + box.w - 2, box.w / 2, l->right, attr) + 1 : 0;
        tui_text(box.y + 1 + i, box.x + 2, box.w - 4 - right, l->text, attr);
    }
    const char *hint = items > 0 ? "\xE2\x86\x91\xE2\x86\x93 select \xC2\xB7 Enter opens \xC2\xB7 Esc close"
                                 : (max_scroll ? "\xE2\x86\x91\xE2\x86\x93 scroll \xC2\xB7 Esc close" : "Esc close");
    tui_text_center(box.y + box.h - 2, box.x, box.w, hint, dim);
}
