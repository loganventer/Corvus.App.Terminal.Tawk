#include "clients/tui/emoji_picker.h"
#include "clients/tui/tui_draw.h"
#include "clients/tui/tui_palette.h"
#include "utilities/str_util.h"
#include "utilities/utf8_text.h"

#include <ctype.h>
#include <ncurses.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>

#define CELL_COLS 4        /* an emoji is two columns wide; two spaces of breathing room */

/* True when a word of `name` starts with `word` ("cat" finds "cat face",
 * not "identification"). */
static int word_prefix(const char *name, const char *word) {
    size_t n = strlen(word);
    for (const char *p = name; *p; p++) {
        int at_word_start = p == name || !isalnum((unsigned char)p[-1]);
        if (at_word_start && strncasecmp(p, word, n) == 0) return 1;
    }
    return 0;
}

/* Every word of the query must start a word of the name ("red heart") or
 * of its keywords ("hug" finds 🤗). */
static int name_matches(const Emoji *e, const char *query) {
    char words[64];
    str_copy(words, sizeof(words), query);
    char *save = NULL;
    for (char *w = strtok_r(words, " ", &save); w; w = strtok_r(NULL, " ", &save)) {
        if (!word_prefix(e->name, w) && !word_prefix(e->keywords, w)) return 0;
    }
    return 1;
}

/* Joined sequences (families, professions, skin-tone pairs) draw at widths
 * terminals disagree on, which breaks the grid; their base emoji are listed. */
static void grid_glyph(const char *glyph, char *out, size_t size);

/* Joined sequences (families, professions) draw at widths terminals disagree
 * on, and emoji newer than the system's character tables have no known
 * width; both would break the grid, so only their base emoji are listed. */
static int grid_safe(const Emoji *e) {
    if (!e || strstr(e->glyph, "\xE2\x80\x8D")) return 0;    /* U+200D zero-width joiner */
    char glyph[48];
    grid_glyph(e->glyph, glyph, sizeof(glyph));
    return utf8_columns(glyph) > 0;
}

/* The glyph as drawn in the grid: without U+FE0F, so the terminal and curses
 * agree on its width. The chosen emoji keeps it. */
static void grid_glyph(const char *glyph, char *out, size_t size) {
    size_t n = 0;
    for (const char *p = glyph; *p && n + 1 < size; ) {
        if ((unsigned char)p[0] == 0xEF && (unsigned char)p[1] == 0xB8 && (unsigned char)p[2] == 0x8F) { p += 3; continue; }
        out[n++] = *p++;
    }
    out[n] = '\0';
}

static void push(EmojiPicker *p, int index) {
    if (p->shown_count < EMOJI_PICKER_MAX_SHOWN) p->shown[p->shown_count++] = index;
}

/* Pads the grid so the next section starts on a new row. */
static void end_row(EmojiPicker *p) {
    int cols = p->cols > 0 ? p->cols : 1;
    while (p->shown_count % cols && p->shown_count < EMOJI_PICKER_MAX_SHOWN) p->shown[p->shown_count++] = -1;
}

static int shown_at(const EmojiPicker *p, int i) {
    return i >= 0 && i < p->shown_count ? p->shown[i] : -1;
}

/* Moves the selection onto a real emoji (off row padding), searching in `dir`. */
static void settle(EmojiPicker *p, int dir) {
    if (p->selected >= p->shown_count) p->selected = p->shown_count - 1;
    if (p->selected < 0) p->selected = 0;
    int i = p->selected;
    while (i >= 0 && i < p->shown_count && p->shown[i] < 0) i += dir;
    if (i < 0 || i >= p->shown_count) {                 /* nothing that way: try the other */
        i = p->selected;
        while (i >= 0 && i < p->shown_count && p->shown[i] < 0) i -= dir;
    }
    p->selected = (i >= 0 && i < p->shown_count) ? i : 0;
}

/* The section the selection is in (for the tab highlight). */
static void follow_section(EmojiPicker *p) {
    if (p->query[0]) return;
    for (int t = EMOJI_PICKER_TABS - 1; t >= 0; t--) {
        if (p->section_start[t] >= 0 && p->selected >= p->section_start[t]) { p->tab = t; return; }
    }
}

/* Rebuilds the grid for the current query and column count, keeping the
 * selected emoji selected. */
static void rebuild_shown(EmojiPicker *p, IEmojiCatalog *catalog) {
    int keep = shown_at(p, p->selected);
    p->shown_count = 0;
    for (int t = 0; t < EMOJI_PICKER_TABS; t++) p->section_start[t] = -1;
    int total = catalog->count(catalog);
    if (p->query[0]) {
        for (int i = 0; i < total; i++) {
            const Emoji *e = catalog->at(catalog, i);
            if (grid_safe(e) && name_matches(e, p->query)) push(p, i);
        }
        p->selected = 0;
        return;
    }
    if (p->recent_count) {
        p->section_start[0] = 0;
        for (int r = 0; r < p->recent_count; r++) {
            int i = catalog->find(catalog, p->recent[r]);
            if (i >= 0 && grid_safe(catalog->at(catalog, i))) push(p, i);
        }
        end_row(p);
    }
    int last_group = -1;
    for (int i = 0; i < total; i++) {
        if (!grid_safe(catalog->at(catalog, i))) continue;
        int g = (int)catalog->at(catalog, i)->group;
        if (g != last_group) {
            if (last_group >= 0) end_row(p);
            if (g + 1 < EMOJI_PICKER_TABS && p->section_start[g + 1] < 0) p->section_start[g + 1] = p->shown_count;
            last_group = g;
        }
        push(p, i);
    }
    /* Keep the same emoji selected (first occurrence after the recent row). */
    p->selected = 0;
    if (keep >= 0) {
        int from = p->section_start[0] >= 0 && p->section_start[1] >= 0 ? p->section_start[1] : 0;
        for (int i = from; i < p->shown_count; i++) if (p->shown[i] == keep) { p->selected = i; break; }
    }
    settle(p, 1);
    follow_section(p);
}

void emoji_picker_open(EmojiPicker *p, IEmojiCatalog *catalog, EmojiPickerPurpose purpose,
                       const char *message_id, const char *recent_csv, const char *query) {
    memset(p, 0, sizeof(*p));
    p->purpose = purpose;
    str_copy(p->message_id, sizeof(p->message_id), message_id ? message_id : "");
    str_copy(p->query, sizeof(p->query), query ? query : "");
    char buf[512];
    str_copy(buf, sizeof(buf), recent_csv ? recent_csv : "");
    char *save = NULL;
    for (char *t = strtok_r(buf, " ", &save); t && p->recent_count < EMOJI_PICKER_RECENT; t = strtok_r(NULL, " ", &save)) {
        str_copy(p->recent[p->recent_count++], sizeof(p->recent[0]), t);
    }
    p->cols = 8;
    p->selected = -1;
    p->open = 1;
    rebuild_shown(p, catalog);
}

/* Jumps to a section and puts its first row at the top of the grid. */
static void jump_to(EmojiPicker *p, IEmojiCatalog *catalog, int tab) {
    if (p->query[0]) { p->query[0] = '\0'; p->selected = -1; rebuild_shown(p, catalog); }
    for (int step = 0; step < EMOJI_PICKER_TABS; step++) {
        int t = ((tab + step) % EMOJI_PICKER_TABS + EMOJI_PICKER_TABS) % EMOJI_PICKER_TABS;
        if (p->section_start[t] < 0) continue;
        p->selected = p->section_start[t];
        p->scroll_row = p->selected / (p->cols > 0 ? p->cols : 1);
        p->tab = t;
        return;
    }
}

/* Next or previous non-empty section from the current one. */
static void step_section(EmojiPicker *p, IEmojiCatalog *catalog, int dir) {
    int t = p->tab;
    for (int i = 0; i < EMOJI_PICKER_TABS; i++) {
        t = (t + dir + EMOJI_PICKER_TABS) % EMOJI_PICKER_TABS;
        if (p->query[0] || p->section_start[t] >= 0) break;
    }
    jump_to(p, catalog, t);
}

PopupResult emoji_picker_key(EmojiPicker *p, IEmojiCatalog *catalog, int is_key, int ch) {
    int cols = p->cols > 0 ? p->cols : 8;
    if (!is_key && ch == 27) {
        if (p->query[0]) { p->query[0] = '\0'; p->selected = -1; rebuild_shown(p, catalog); return POPUP_CHANGED; }
        p->open = 0;
        return POPUP_CLOSED;
    }
    if ((!is_key && (ch == '\n' || ch == '\r')) || (is_key && ch == KEY_ENTER)) {
        if (shown_at(p, p->selected) < 0) return POPUP_NONE;
        p->open = 0;
        return POPUP_CHOSEN;
    }
    if (!is_key && ch == '\t') { step_section(p, catalog, 1); return POPUP_CHANGED; }
    if (is_key) {
        int dir = 1;
        switch (ch) {
            case KEY_BTAB:  step_section(p, catalog, -1); return POPUP_CHANGED;
            case KEY_LEFT:  p->selected--; dir = -1; break;
            case KEY_RIGHT: p->selected++; break;
            case KEY_UP:    p->selected -= cols; dir = -1; break;
            case KEY_DOWN:  p->selected += cols; break;
            case KEY_PPAGE: p->selected -= cols * (p->grid.h > 1 ? p->grid.h - 1 : 5); dir = -1; break;
            case KEY_NPAGE: p->selected += cols * (p->grid.h > 1 ? p->grid.h - 1 : 5); break;
            case KEY_HOME:  p->selected = 0; break;
            case KEY_END:   p->selected = p->shown_count - 1; dir = -1; break;
            case KEY_BACKSPACE: {
                size_t len = strlen(p->query);
                if (len) { p->query[len - 1] = '\0'; p->selected = -1; rebuild_shown(p, catalog); }
                return POPUP_CHANGED;
            }
            default: return POPUP_NONE;
        }
        settle(p, dir);
        follow_section(p);
        return POPUP_CHANGED;
    }
    size_t len = strlen(p->query);
    if (ch == 127 || ch == 8) {
        if (len) { p->query[len - 1] = '\0'; p->selected = -1; rebuild_shown(p, catalog); }
        return POPUP_CHANGED;
    }
    if (ch >= 32 && ch < 127 && len + 1 < sizeof(p->query)) {
        if (ch == ' ' && len == 0) return POPUP_NONE;    /* a leading space is not a search */
        p->query[len] = (char)tolower(ch);
        p->query[len + 1] = '\0';
        p->scroll_row = 0;
        rebuild_shown(p, catalog);
        return POPUP_CHANGED;
    }
    return POPUP_NONE;
}

PopupResult emoji_picker_click(EmojiPicker *p, IEmojiCatalog *catalog, int y, int x) {
    if (!ui_rect_contains(p->last_rect, y, x)) { p->open = 0; return POPUP_CLOSED; }
    if (y == p->last_rect.y + 2) {                      /* tabs */
        for (int t = 0; t < EMOJI_PICKER_TABS; t++) {
            if (x >= p->tab_x[t] && x < p->tab_x[t + 1]) { jump_to(p, catalog, t); return POPUP_CHANGED; }
        }
        return POPUP_NONE;
    }
    if (!ui_rect_contains(p->grid, y, x)) return POPUP_NONE;
    int index = (p->scroll_row + (y - p->grid.y)) * p->cols + (x - p->grid.x) / CELL_COLS;
    if (shown_at(p, index) < 0) return POPUP_NONE;
    p->selected = index;
    p->open = 0;
    return POPUP_CHOSEN;
}

void emoji_picker_wheel(EmojiPicker *p, int delta) {
    int rows = (p->shown_count + p->cols - 1) / (p->cols ? p->cols : 1);
    p->scroll_row += delta * 2;
    if (p->scroll_row > rows - p->grid.h) p->scroll_row = rows - p->grid.h;
    if (p->scroll_row < 0) p->scroll_row = 0;
    /* keep the selection inside the visible rows */
    int first = p->scroll_row * p->cols, last = (p->scroll_row + p->grid.h) * p->cols - 1;
    int dir = delta > 0 ? 1 : -1;
    if (p->selected < first) { p->selected = first; dir = 1; }
    if (p->selected > last) { p->selected = last; dir = -1; }
    settle(p, dir);
    follow_section(p);
}

const char *emoji_picker_choice(const EmojiPicker *p, IEmojiCatalog *catalog) {
    int index = shown_at(p, p->selected);
    if (index < 0) return "";
    const Emoji *e = catalog->at(catalog, index);
    return e ? e->glyph : "";
}

void emoji_picker_render(EmojiPicker *p, UiRect area, IEmojiCatalog *catalog) {
    int w = area.w < 64 ? area.w : 64;
    int h = area.h < 26 ? area.h : 26;
    UiRect box = { area.y + area.h - h, area.x + (area.w - w) / 2, h, w };
    p->last_rect = box;
    int base = tui_palette_attr(THEME_SLOT_BASE);
    tui_box(box, p->purpose == EMOJI_PICKER_FOR_REACTION ? "React with" : "Emoji", tui_palette_attr(THEME_SLOT_BORDER));
    tui_fill((UiRect){ box.y + 1, box.x + 1, box.h - 2, box.w - 2 }, base);

    /* Search line */
    char field[96];
    snprintf(field, sizeof(field), " \xF0\x9F\x94\x8D %s", p->query);
    tui_fill((UiRect){ box.y + 1, box.x + 1, 1, box.w - 2 }, tui_palette_attr(THEME_SLOT_COMPOSER));
    int used = tui_text(box.y + 1, box.x + 1, box.w - 2, field, tui_palette_attr(THEME_SLOT_COMPOSER) | ATTR_BOLD);
    if (!p->query[0]) tui_text(box.y + 1, box.x + 1 + used, box.w - 2 - used, "type to search, e.g. heart, cat, party",
                               tui_palette_attr(THEME_SLOT_COMPOSER) | ATTR_DIM);
    p->caret = (TextCaret){ 1, box.y + 1, box.x + 1 + used };

    /* Tabs: Recent, then one icon per group */
    int x = box.x + 2;
    for (int t = 0; t < EMOJI_PICKER_TABS; t++) {
        p->tab_x[t] = x;
        const char *icon = t == 0 ? "\xF0\x9F\x95\x92" : emoji_group_icon((EmojiGroup)(t - 1));
        char cell[16];
        snprintf(cell, sizeof(cell), " %s ", icon);
        int active = !p->query[0] && t == p->tab;
        x += tui_text(box.y + 2, x, box.x + box.w - 1 - x, cell,
                      active ? tui_palette_attr(THEME_SLOT_BADGE) | ATTR_BOLD : base);
    }
    p->tab_x[EMOJI_PICKER_TABS] = x;

    /* Grid */
    p->grid = (UiRect){ box.y + 4, box.x + 2, box.h - 7, box.w - 4 };
    int cols = p->grid.w / CELL_COLS;
    if (cols < 1) cols = 1;
    if (cols != p->cols) { p->cols = cols; rebuild_shown(p, catalog); }   /* sections start on new rows */
    int sel_row = p->selected / p->cols;
    if (sel_row < p->scroll_row) p->scroll_row = sel_row;
    if (sel_row >= p->scroll_row + p->grid.h) p->scroll_row = sel_row - p->grid.h + 1;
    if (!p->shown_count) tui_text_center(p->grid.y + 1, box.x, box.w, "No emoji match", base | ATTR_DIM);
    for (int r = 0; r < p->grid.h; r++) {
        for (int c = 0; c < p->cols; c++) {
            int i = (p->scroll_row + r) * p->cols + c;
            if (i >= p->shown_count) break;
            if (p->shown[i] < 0) continue;                       /* row padding between groups */
            const Emoji *e = catalog->at(catalog, p->shown[i]);
            int attr = i == p->selected ? tui_palette_attr(THEME_SLOT_SIDEBAR_SELECTED) | ATTR_BOLD : base;
            tui_fill((UiRect){ p->grid.y + r, p->grid.x + c * CELL_COLS, 1, CELL_COLS }, attr);
            char glyph[48];
            grid_glyph(e->glyph, glyph, sizeof(glyph));
            tui_text(p->grid.y + r, p->grid.x + c * CELL_COLS + 1, 2, glyph, attr);
        }
    }

    /* Name of the selected emoji, and position */
    char footer[160];
    int sel_index = shown_at(p, p->selected);
    const Emoji *sel = sel_index >= 0 ? catalog->at(catalog, sel_index) : NULL;
    int rows_total = (p->shown_count + p->cols - 1) / p->cols;
    char sel_glyph[48] = "";
    if (sel) grid_glyph(sel->glyph, sel_glyph, sizeof(sel_glyph));
    snprintf(footer, sizeof(footer), " %s  %s", sel_glyph, sel ? sel->name : "");
    tui_text(box.y + box.h - 3, box.x + 1, box.w - 12, footer, base | ATTR_BOLD);
    if (rows_total > p->grid.h) {                    /* a scrollbar thumb on the right edge */
        int thumb = p->grid.h * p->grid.h / rows_total;
        if (thumb < 1) thumb = 1;
        int top = (rows_total - p->grid.h) > 0 ? p->scroll_row * (p->grid.h - thumb) / (rows_total - p->grid.h) : 0;
        for (int r = 0; r < p->grid.h; r++) {
            tui_text(p->grid.y + r, box.x + box.w - 2, 1, r >= top && r < top + thumb ? "\xE2\x94\x83" : "\xE2\x94\x82",
                     r >= top && r < top + thumb ? tui_palette_attr(THEME_SLOT_ACCENT) : tui_palette_attr(THEME_SLOT_DIM));
        }
    }
    tui_text_center(box.y + box.h - 2, box.x, box.w,
                    "type to search \xC2\xB7 Tab next group \xC2\xB7 Enter pick \xC2\xB7 Esc close",
                    tui_palette_attr(THEME_SLOT_DIM));
}
