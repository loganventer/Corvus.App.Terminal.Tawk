#include "clients/tui/settings_panel.h"
#include "clients/tui/toggle_switch.h"
#include "clients/tui/settings_menu.h"
#include "clients/tui/tui_draw.h"
#include "clients/tui/tui_palette.h"
#include "core/settings_schema.h"
#include "utilities/str_util.h"
#include "utilities/utf8_text.h"

#include <ncurses.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void settings_panel_init(SettingsPanel *p, SettingsPanelHost host) {
    memset(p, 0, sizeof(*p));
    p->host = host;
}

void settings_panel_open(SettingsPanel *p) {
    p->open = 1;
    p->depth = 1;
    p->stack[0] = settings_menu_root();
    p->selected[0] = p->scroll[0] = 0;
    p->picking_theme = p->editing = 0;
}

static void end_theme_pick(SettingsPanel *p, int keep) {
    if (!p->picking_theme) return;
    IThemeRepository *repo = p->host.themes(p->host.ctx);
    if (keep) {
        Settings s = *p->host.settings(p->host.ctx);
        str_copy(s.theme, sizeof(s.theme), repo->at(repo, p->theme_index)->id);
        p->host.apply(p->host.ctx, &s);
    } else {
        int i = repo->index_of(repo, p->theme_before);
        p->host.preview_theme(p->host.ctx, repo->at(repo, i < 0 ? 0 : i));
    }
    p->picking_theme = 0;
}

void settings_panel_close(SettingsPanel *p) {
    end_theme_pick(p, 0);
    p->open = 0;
}

static const MenuNode *current(SettingsPanel *p) {
    int agent = p->host.agent_connected ? p->host.agent_connected(p->host.ctx) : 0;
    return settings_menu_shown(p->stack[p->depth - 1], agent);
}

static const SettingField *field_of(const MenuNode *n) {
    return n->kind == MENU_NODE_FIELD ? settings_schema_find(n->category, n->key) : NULL;
}

static void toast(SettingsPanel *p, const char *text, long long now) {
    str_copy(p->toast, sizeof(p->toast), text);
    p->toast_until_ms = now + 2500;
}

static void save(SettingsPanel *p, const Settings *s, const SettingField *f, long long now) {
    if (p->host.apply(p->host.ctx, s) == 0) {
        toast(p, f && f->requires_restart ? "Saved. Restart tawk to use this setting." : "Saved", now);
    } else {
        toast(p, "Could not save the config file", now);
    }
}

static void cycle_choice(Settings *s, const SettingField *f, int dir) {
    char choices[128], *opts[16];
    int n = 0, cur = 0;
    str_copy(choices, sizeof(choices), f->choices);
    char *save_ptr = NULL;
    for (char *t = strtok_r(choices, "|", &save_ptr); t && n < 16; t = strtok_r(NULL, "|", &save_ptr)) {
        if (strcmp(t, setting_get_string(s, f)) == 0) cur = n;
        opts[n++] = t;
    }
    if (n) setting_set_string(s, f, opts[(cur + dir + n) % n]);
}

/* Left/right or click on a field: toggle, step, or cycle. */
static void adjust(SettingsPanel *p, const SettingField *f, int dir, long long now) {
    Settings s = *p->host.settings(p->host.ctx);
    switch (f->kind) {
        case SETTING_KIND_BOOL:   setting_set_int(&s, f, !setting_get_int(&s, f)); break;
        case SETTING_KIND_INT:    setting_set_int(&s, f, setting_get_int(&s, f) + dir * f->step); break;
        case SETTING_KIND_CHOICE: cycle_choice(&s, f, dir); break;
        default: return;
    }
    save(p, &s, f, now);
}

static void begin_edit(SettingsPanel *p, const SettingField *f) {
    const char *value = setting_get_string(p->host.settings(p->host.ctx), f);
    size_t n = mbstowcs(p->edit, value, sizeof(p->edit) / sizeof(p->edit[0]) - 1);
    p->edit_len = n == (size_t)-1 ? 0 : (int)n;
    p->edit[p->edit_len] = L'\0';
    p->editing = 1;
}

static void commit_edit(SettingsPanel *p, const SettingField *f, long long now) {
    char *text = utf8_from_wide(p->edit, (size_t)p->edit_len);
    if (text) {
        Settings s = *p->host.settings(p->host.ctx);
        setting_set_string(&s, f, str_trim(text));
        save(p, &s, f, now);
        free(text);
    }
    p->editing = 0;
}

static void begin_theme_pick(SettingsPanel *p) {
    IThemeRepository *repo = p->host.themes(p->host.ctx);
    const Settings *s = p->host.settings(p->host.ctx);
    str_copy(p->theme_before, sizeof(p->theme_before), s->theme);
    int i = repo->index_of(repo, s->theme);
    p->theme_index = i < 0 ? 0 : i;
    p->theme_scroll = 0;
    p->picking_theme = 1;
}

static void activate(SettingsPanel *p, int index, long long now) {
    const MenuNode *menu = current(p);
    if (index < 0 || index >= menu->child_count) return;
    p->selected[p->depth - 1] = index;
    const MenuNode *n = &menu->children[index];
    switch (n->kind) {
        case MENU_NODE_SUBMENU:
            if (p->depth < SETTINGS_PANEL_DEPTH) {
                p->stack[p->depth] = n;
                p->selected[p->depth] = p->scroll[p->depth] = 0;
                p->depth++;
            }
            break;
        case MENU_NODE_FIELD: {
            const SettingField *f = field_of(n);
            if (!f) break;
            if (f->kind == SETTING_KIND_STRING) begin_edit(p, f);
            else adjust(p, f, 1, now);
            break;
        }
        case MENU_NODE_THEMES:
            begin_theme_pick(p);
            break;
        case MENU_NODE_ACTION:
            p->host.run_action(p->host.ctx, n->action);
            break;
        default:
            break;
    }
}

/* Returns 1 when the panel should close. */
static int go_back(SettingsPanel *p) {
    if (p->picking_theme) { end_theme_pick(p, 0); return 0; }
    if (p->editing) { p->editing = 0; return 0; }
    if (p->depth > 1) { p->depth--; return 0; }
    settings_panel_close(p);
    return 1;
}

static void preview_selected_theme(SettingsPanel *p) {
    IThemeRepository *repo = p->host.themes(p->host.ctx);
    p->host.preview_theme(p->host.ctx, repo->at(repo, p->theme_index));
}

int settings_panel_key(SettingsPanel *p, int is_key, int ch, long long now) {
    if (!p->open) return 1;
    int esc = !is_key && ch == 27;
    int back = esc || (is_key && ch == KEY_BACKSPACE && !p->editing) || (!is_key && (ch == 127 || ch == 8) && !p->editing);

    if (p->editing) {
        const SettingField *f = field_of(&current(p)->children[p->selected[p->depth - 1]]);
        if (esc) { p->editing = 0; return 0; }
        if (!is_key && (ch == '\n' || ch == '\r')) { if (f) commit_edit(p, f, now); return 0; }
        if ((is_key && ch == KEY_BACKSPACE) || (!is_key && (ch == 127 || ch == 8))) {
            if (p->edit_len > 0) p->edit[--p->edit_len] = L'\0';
            return 0;
        }
        if (!is_key && ch >= 32 && p->edit_len < (int)(sizeof(p->edit) / sizeof(p->edit[0])) - 1) {
            p->edit[p->edit_len++] = (wchar_t)ch;
            p->edit[p->edit_len] = L'\0';
        }
        return 0;
    }

    if (p->picking_theme) {
        IThemeRepository *repo = p->host.themes(p->host.ctx);
        int n = repo->count(repo);
        if (is_key && (ch == KEY_UP || ch == KEY_DOWN || ch == KEY_PPAGE || ch == KEY_NPAGE || ch == KEY_HOME || ch == KEY_END)) {
            int step = ch == KEY_UP ? -1 : ch == KEY_DOWN ? 1 : ch == KEY_PPAGE ? -10 : ch == KEY_NPAGE ? 10 : ch == KEY_HOME ? -n : n;
            p->theme_index += step;
            if (p->theme_index < 0) p->theme_index = 0;
            if (p->theme_index >= n) p->theme_index = n - 1;
            preview_selected_theme(p);
        } else if (!is_key && (ch == '\n' || ch == '\r' || ch == ' ')) {
            end_theme_pick(p, 1);
            toast(p, "Theme applied", now);
        } else if (back) {
            go_back(p);
        }
        return 0;
    }

    const MenuNode *menu = current(p);
    int *sel = &p->selected[p->depth - 1];
    if (back) return go_back(p);
    if (is_key && ch == KEY_UP && *sel > 0) (*sel)--;
    else if (is_key && ch == KEY_DOWN && *sel < menu->child_count - 1) (*sel)++;
    else if (is_key && ch == KEY_HOME) *sel = 0;
    else if (is_key && ch == KEY_END) *sel = menu->child_count - 1;
    else if (is_key && (ch == KEY_LEFT || ch == KEY_RIGHT)) {
        const SettingField *f = field_of(&menu->children[*sel]);
        if (f && f->kind != SETTING_KIND_STRING) adjust(p, f, ch == KEY_RIGHT ? 1 : -1, now);
        else if (ch == KEY_LEFT) return go_back(p);
        else activate(p, *sel, now);
    } else if (!is_key && (ch == '\n' || ch == '\r' || ch == ' ')) {
        activate(p, *sel, now);
    } else if (!is_key && ch == 'q') {
        settings_panel_close(p);
        return 1;
    }
    return 0;
}

void settings_panel_paste(SettingsPanel *p, const char *utf8) {
    if (!p->editing || !utf8) return;
    wchar_t buf[512];
    size_t n = mbstowcs(buf, utf8, 511);
    if (n == (size_t)-1) return;
    for (size_t i = 0; i < n && p->edit_len < 511; i++) {
        if (buf[i] >= 32) p->edit[p->edit_len++] = buf[i];
    }
    p->edit[p->edit_len] = L'\0';
}

void settings_panel_wheel(SettingsPanel *p, int delta) {
    if (p->picking_theme) {
        IThemeRepository *repo = p->host.themes(p->host.ctx);
        p->theme_index += delta;
        if (p->theme_index < 0) p->theme_index = 0;
        if (p->theme_index >= repo->count(repo)) p->theme_index = repo->count(repo) - 1;
        preview_selected_theme(p);
        return;
    }
    const MenuNode *menu = current(p);
    int *sel = &p->selected[p->depth - 1];
    *sel += delta;
    if (*sel < 0) *sel = 0;
    if (*sel >= menu->child_count) *sel = menu->child_count - 1;
}

int settings_panel_click(SettingsPanel *p, int y, int x, long long now) {
    UiRect r = p->last_rect;
    if (!ui_rect_contains(r, y, x)) return 0;
    if (y == r.y + 1) {                               /* breadcrumb: jump back up */
        for (int d = 0; d < p->depth; d++) {
            if (x >= p->crumb_x[d] && x < p->crumb_x[d + 1]) {
                end_theme_pick(p, 0);
                p->editing = 0;
                p->depth = d + 1;
                return 0;
            }
        }
        return 0;
    }
    int k = y - r.y;
    if (k < 0 || k >= SETTINGS_PANEL_ROWS || p->row_item[k] < 0) return 0;
    int item = p->row_item[k];
    if (p->picking_theme) {
        if (item == p->theme_index) { end_theme_pick(p, 1); toast(p, "Theme applied", now); }
        else { p->theme_index = item; preview_selected_theme(p); }
        return 0;
    }
    const MenuNode *n = &current(p)->children[item];
    const SettingField *f = field_of(n);
    p->selected[p->depth - 1] = item;
    if (f && f->kind == SETTING_KIND_INT) {
        adjust(p, f, x < r.x + r.w - 8 ? -1 : 1, now);   /* click the ‹ or › side */
        return 0;
    }
    activate(p, item, now);
    return 0;
}

/* ---- rendering ---------------------------------------------------------- */

static void value_text(SettingsPanel *p, const MenuNode *n, char *out, size_t size) {
    out[0] = '\0';
    if (n->kind == MENU_NODE_INFO && n->info != MENU_INFO_STATIC) {
        p->host.info(p->host.ctx, n->info, out, size);
        return;
    }
    const SettingField *f = field_of(n);
    if (!f) return;
    const Settings *s = p->host.settings(p->host.ctx);
    switch (f->kind) {
        case SETTING_KIND_BOOL:   snprintf(out, size, "%s", toggle_switch_text(setting_get_int(s, f))); break;
        case SETTING_KIND_INT:    snprintf(out, size, "\xE2\x80\xB9 %d \xE2\x80\xBA", setting_get_int(s, f)); break;
        case SETTING_KIND_CHOICE: snprintf(out, size, "\xE2\x80\xB9 %s \xE2\x80\xBA", setting_get_string(s, f)); break;
        default: {
            const char *v = setting_get_string(s, f);
            snprintf(out, size, "%s", v[0] ? v : "(default)");
            break;
        }
    }
}

static void render_breadcrumb(SettingsPanel *p, UiRect r) {
    int x = r.x + 2;
    int attr = tui_palette_attr(THEME_SLOT_HEADER);
    tui_fill((UiRect){ r.y + 1, r.x + 1, 1, r.w - 2 }, attr);
    for (int d = 0; d < p->depth; d++) {
        p->crumb_x[d] = x;
        if (d) x += tui_text(r.y + 1, x, r.w - (x - r.x) - 2, " \xE2\x80\xBA ", attr | ATTR_DIM);
        x += tui_text(r.y + 1, x, r.w - (x - r.x) - 2, p->stack[d]->title, attr | (d == p->depth - 1 ? ATTR_BOLD : 0));
    }
    p->crumb_x[p->depth] = x;
    if (p->picking_theme) tui_text(r.y + 1, x, r.w - (x - r.x) - 2, " \xE2\x80\xBA Choose theme", attr | ATTR_BOLD);
}

static void render_themes(SettingsPanel *p, UiRect list) {
    IThemeRepository *repo = p->host.themes(p->host.ctx);
    int n = repo->count(repo);
    if (p->theme_index < p->theme_scroll) p->theme_scroll = p->theme_index;
    if (p->theme_index >= p->theme_scroll + list.h) p->theme_scroll = p->theme_index - list.h + 1;
    for (int k = 0; k < list.h && p->theme_scroll + k < n; k++) {
        int i = p->theme_scroll + k;
        const Theme *t = repo->at(repo, i);
        int sel = i == p->theme_index;
        int attr = sel ? tui_palette_attr(THEME_SLOT_SIDEBAR_SELECTED) | ATTR_BOLD : tui_palette_attr(THEME_SLOT_BASE);
        tui_fill((UiRect){ list.y + k, list.x, 1, list.w }, attr);
        char line[256];
        snprintf(line, sizeof(line), " %s %-22s %s", strcmp(t->id, p->theme_before) == 0 ? "\xE2\x9C\x93" : " ", t->name, t->description);
        tui_text(list.y + k, list.x, list.w, line, attr);
        if (list.y + k - p->last_rect.y < SETTINGS_PANEL_ROWS) p->row_item[list.y + k - p->last_rect.y] = i;
    }
}

static void render_items(SettingsPanel *p, UiRect list) {
    const MenuNode *menu = current(p);
    int sel = p->selected[p->depth - 1];
    int *scroll = &p->scroll[p->depth - 1];
    int per = 2;                                               /* title + subtitle/value */
    int slots = list.h / per;
    if (slots < 1) slots = 1;
    if (sel < *scroll) *scroll = sel;
    if (sel >= *scroll + slots) *scroll = sel - slots + 1;
    for (int k = 0; k < slots && *scroll + k < menu->child_count; k++) {
        int i = *scroll + k;
        const MenuNode *n = &menu->children[i];
        const SettingField *f = field_of(n);
        int y = list.y + k * per;
        int selected = i == sel;
        int attr = selected ? tui_palette_attr(THEME_SLOT_SIDEBAR_SELECTED) : tui_palette_attr(THEME_SLOT_BASE);
        tui_fill((UiRect){ y, list.x, per, list.w }, attr);
        const char *title = f ? f->label : n->title;
        const char *sub = f ? f->help : n->subtitle;
        char head[192];
        snprintf(head, sizeof(head), " %s %s%s", n->icon && *n->icon ? n->icon : " ", title,
                 n->kind == MENU_NODE_SUBMENU ? "  \xE2\x80\xBA" : "");
        char value[512];
        value_text(p, n, value, sizeof(value));
        int value_cols = 0;
        if (p->editing && selected) {
            char *text = utf8_from_wide(p->edit, (size_t)p->edit_len);
            char shown[600];
            snprintf(shown, sizeof(shown), " %s", text ? text : "");
            free(text);
            tui_fill((UiRect){ y + 1, list.x + 3, 1, list.w - 5 }, tui_palette_attr(THEME_SLOT_COMPOSER));
            int used = tui_text(y + 1, list.x + 3, list.w - 5, shown, tui_palette_attr(THEME_SLOT_COMPOSER) | ATTR_BOLD);
            p->caret = (TextCaret){ 1, y + 1, list.x + 3 + used };
        } else if (sub) {
            char s2[256];
            snprintf(s2, sizeof(s2), "    %s", sub);
            tui_text(y + 1, list.x, list.w - 1, s2, attr | ATTR_DIM);
        }
        if (value[0] && !(p->editing && selected)) {
            int vattr = attr | ATTR_BOLD;
            if (f && f->kind == SETTING_KIND_BOOL && setting_get_int(p->host.settings(p->host.ctx), f)) vattr = tui_palette_attr(THEME_SLOT_ACCENT) | ATTR_BOLD;
            value_cols = tui_text_right(y, list.x + list.w - 2, list.w / 2, value, vattr);
        }
        tui_text(y, list.x, list.w - value_cols - 3, head, attr | (n->kind == MENU_NODE_INFO && n->info == MENU_INFO_STATIC ? 0 : ATTR_BOLD));
        for (int r = 0; r < per; r++) {
            if (y + r - p->last_rect.y < SETTINGS_PANEL_ROWS) p->row_item[y + r - p->last_rect.y] = i;
        }
    }
}

void settings_panel_render(SettingsPanel *p, UiRect r, long long now) {
    p->last_rect = r;
    p->caret.visible = 0;
    for (int i = 0; i < SETTINGS_PANEL_ROWS; i++) p->row_item[i] = -1;
    tui_box(r, "\xE2\x9A\x99 Settings", tui_palette_attr(THEME_SLOT_BORDER));
    tui_fill((UiRect){ r.y + 1, r.x + 1, r.h - 2, r.w - 2 }, tui_palette_attr(THEME_SLOT_BASE));
    render_breadcrumb(p, r);
    UiRect list = { r.y + 3, r.x + 1, r.h - 6, r.w - 2 };
    if (p->picking_theme) render_themes(p, list);
    else render_items(p, list);

    const char *hint = p->picking_theme ? "\xE2\x86\x91\xE2\x86\x93 preview \xC2\xB7 Enter apply \xC2\xB7 Esc cancel"
                     : p->editing ? "Type or drop a path \xC2\xB7 Enter save \xC2\xB7 Esc cancel"
                     : "\xE2\x86\x91\xE2\x86\x93 move \xC2\xB7 Enter open/toggle \xC2\xB7 \xE2\x86\x90\xE2\x86\x92 adjust \xC2\xB7 Esc back";
    if (p->toast[0] && now < p->toast_until_ms) {
        tui_text_center(r.y + r.h - 2, r.x, r.w, p->toast, tui_palette_attr(THEME_SLOT_ACCENT) | ATTR_BOLD);
    } else {
        tui_text_center(r.y + r.h - 2, r.x, r.w, hint, tui_palette_attr(THEME_SLOT_DIM));
    }
}
