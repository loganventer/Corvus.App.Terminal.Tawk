#include "clients/tui/scheduled_list_dialog.h"
#include "clients/tui/tui_draw.h"
#include "clients/tui/tui_palette.h"
#include "utilities/clock_util.h"
#include "utilities/str_util.h"
#include "utilities/utf8_text.h"

#include <ncurses.h>
#include <stdio.h>
#include <string.h>

#define WIDTH 76
#define DOT   "\xC2\xB7"

void scheduled_list_dialog_open(ScheduledListDialog *d) {
    memset(d, 0, sizeof(*d));
    text_field_init(&d->when, 40);
    d->open = 1;
}

const char *scheduled_list_dialog_selected(const ScheduledListDialog *d) {
    return d->selected >= 0 && d->selected < d->count ? d->ids[d->selected] : NULL;
}

char *scheduled_list_dialog_when(const ScheduledListDialog *d) { return text_field_text(&d->when); }

void scheduled_list_dialog_rescheduled(ScheduledListDialog *d) {
    d->editing = 0;
    d->error[0] = '\0';
}

void scheduled_list_dialog_error(ScheduledListDialog *d, const char *why) { str_copy(d->error, sizeof(d->error), why ? why : ""); }

static ScheduledListRequest close_it(ScheduledListDialog *d) {
    d->open = 0;
    d->caret.visible = 0;
    return SCHEDULED_REQUEST_CLOSED;
}

static ScheduledListRequest start_editing(ScheduledListDialog *d) {
    if (!scheduled_list_dialog_selected(d)) return SCHEDULED_REQUEST_NONE;
    d->editing = 1;
    d->error[0] = '\0';
    text_field_set(&d->when, "");
    return SCHEDULED_REQUEST_REDRAW;
}

/* Something to act on only when a message is highlighted. */
static ScheduledListRequest on_selected(const ScheduledListDialog *d, ScheduledListRequest request) {
    return scheduled_list_dialog_selected(d) ? request : SCHEDULED_REQUEST_NONE;
}

ScheduledListRequest scheduled_list_dialog_key(ScheduledListDialog *d, int is_key, int ch) {
    int enter = (!is_key && (ch == '\n' || ch == '\r')) || (is_key && ch == KEY_ENTER);
    if (d->editing) {
        if (!is_key && ch == 27) { d->editing = 0; d->error[0] = '\0'; return SCHEDULED_REQUEST_REDRAW; }
        if (enter) return on_selected(d, SCHEDULED_REQUEST_RESCHEDULE);
        if (text_field_key(&d->when, is_key, ch)) { d->error[0] = '\0'; return SCHEDULED_REQUEST_REDRAW; }
        return SCHEDULED_REQUEST_NONE;
    }
    if (!is_key && (ch == 27 || ch == 'q')) return close_it(d);
    if (is_key && ch == KEY_UP && d->selected > 0) { d->selected--; return SCHEDULED_REQUEST_REDRAW; }
    if (is_key && ch == KEY_DOWN && d->selected < d->count - 1) { d->selected++; return SCHEDULED_REQUEST_REDRAW; }
    if (enter || (!is_key && ch == 's')) return on_selected(d, SCHEDULED_REQUEST_SEND_NOW);
    if (!is_key && (ch == 't' || ch == 'e')) return start_editing(d);
    if ((is_key && ch == KEY_DC) || (!is_key && (ch == 'c' || ch == 'x'))) return on_selected(d, SCHEDULED_REQUEST_CANCEL);
    return SCHEDULED_REQUEST_NONE;
}

ScheduledListRequest scheduled_list_dialog_click(ScheduledListDialog *d, int y, int x) {
    if (!ui_rect_contains(d->last_rect, y, x)) return close_it(d);
    if (ui_rect_contains(d->send_button, y, x)) return on_selected(d, SCHEDULED_REQUEST_SEND_NOW);
    if (ui_rect_contains(d->time_button, y, x)) return start_editing(d);
    if (ui_rect_contains(d->cancel_button, y, x)) return on_selected(d, SCHEDULED_REQUEST_CANCEL);
    if (ui_rect_contains(d->list_rect, y, x)) {
        int row = d->scroll + (y - d->list_rect.y) / 2;      /* two lines per message */
        if (row >= 0 && row < d->count) d->selected = row;
        return SCHEDULED_REQUEST_REDRAW;
    }
    return SCHEDULED_REQUEST_NONE;
}

/* A button; returns where it was drawn. */
static UiRect button(int y, int x, const char *label, int attr) {
    int w = utf8_columns(label);
    tui_text(y, x, w, label, attr);
    return (UiRect){ y, x, 1, w };
}

void scheduled_list_dialog_render(ScheduledListDialog *d, UiRect a, const ScheduledMessage *items, int count,
                                  void (*name_of)(void *ctx, const char *jid, char *out, size_t size), void *ctx, int use_24h) {
    d->count = count < SCHEDULED_LIST_ROWS ? count : SCHEDULED_LIST_ROWS;
    for (int i = 0; i < d->count; i++) str_copy(d->ids[i], sizeof(d->ids[0]), items[i].id);
    if (d->selected >= d->count) d->selected = d->count > 0 ? d->count - 1 : 0;

    int w = a.w < WIDTH ? a.w : WIDTH;
    int h = a.h < 26 ? a.h : 26;
    UiRect box = { a.y + (a.h - h) / 2, a.x + (a.w - w) / 2, h, w };
    d->last_rect = box;
    int base = tui_palette_attr(THEME_SLOT_BASE);
    tui_fill(box, base);
    tui_box(box, "Scheduled messages", tui_palette_attr(THEME_SLOT_BORDER));

    UiRect list = { box.y + 1, box.x + 1, box.h - 7, box.w - 2 };
    d->list_rect = list;
    int visible = list.h / 2 > 0 ? list.h / 2 : 1;
    if (d->selected < d->scroll) d->scroll = d->selected;
    if (d->selected >= d->scroll + visible) d->scroll = d->selected - visible + 1;
    if (d->count == 0) {
        tui_text_center(list.y + 2, list.x, list.w, "Nothing is waiting to be sent.", base | ATTR_DIM);
        tui_text_center(list.y + 3, list.x, list.w, "In a chat, type /later 18:00 and your message.", base | ATTR_DIM);
    }
    for (int r = 0; r < visible && d->scroll + r < d->count; r++) {
        int k = d->scroll + r, y = list.y + r * 2;
        const ScheduledMessage *s = &items[k];
        int attr = k == d->selected ? tui_palette_attr(THEME_SLOT_SIDEBAR_SELECTED) : base;
        tui_fill((UiRect){ y, list.x, 2, list.w }, attr);
        char when[48], who[128], head[220];
        clock_format_upcoming(s->due_at, use_24h, when, sizeof(when));
        who[0] = '\0';
        if (name_of) name_of(ctx, s->chat_jid, who, sizeof(who));
        snprintf(head, sizeof(head), "\xF0\x9F\x95\x93 %s " DOT " %s", when, who[0] ? who : s->chat_jid);   /* 🕓 */
        tui_text(y, list.x + 1, list.w - 2, head, attr | ATTR_BOLD);
        char line[300];
        str_copy(line, sizeof(line), s->text ? s->text : "");
        for (char *p = line; *p; p++) if (*p == '\n' || *p == '\t') *p = ' ';
        tui_text(y + 1, list.x + 3, list.w - 4, line, attr | ATTR_DIM);
    }

    int y = box.y + box.h - 5;
    d->caret.visible = 0;
    if (d->editing) {
        tui_text(y, box.x + 2, 20, "New time:", base | ATTR_BOLD);
        UiRect field = { y, box.x + 12, 1, box.w - 14 };
        text_field_render(&d->when, field, tui_palette_attr(THEME_SLOT_COMPOSER), 1, &d->caret);
        if (text_field_length(&d->when) == 0) tui_text(field.y, field.x, field.w, "18:00, +1h, tomorrow 9:00, fri 17:30",
                                                      tui_palette_attr(THEME_SLOT_COMPOSER) | ATTR_DIM);
    }
    if (d->error[0]) tui_text(y + 1, box.x + 2, box.w - 4, d->error, tui_palette_attr(THEME_SLOT_WARN) | ATTR_BOLD);

    int by = box.y + box.h - 2, bx = box.x + 2;
    int accent = tui_palette_attr(THEME_SLOT_ACCENT) | ATTR_BOLD;
    d->send_button = button(by, bx, "[ Send now ]", accent);
    d->time_button = button(by, bx + d->send_button.w + 2, "[ Change time ]", accent);
    d->cancel_button = button(by, bx + d->send_button.w + d->time_button.w + 4, "[ Cancel message ]", tui_palette_attr(THEME_SLOT_WARN) | ATTR_BOLD);
    const char *hint = d->editing ? " Enter set time " DOT " Esc back "
                                  : " Enter send now " DOT " T change time " DOT " Del cancel " DOT " Esc close ";
    tui_text_center(box.y + box.h - 1, box.x, box.w, hint, tui_palette_attr(THEME_SLOT_BORDER));
}
