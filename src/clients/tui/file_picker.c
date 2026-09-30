#include "core/icon_glyphs.h"
#include "clients/tui/file_picker.h"
#include "clients/tui/tui_draw.h"
#include "clients/tui/tui_palette.h"
#include "utilities/dir_listing.h"
#include "utilities/path_util.h"
#include "utilities/platform.h"
#include "utilities/str_util.h"

#include <dirent.h>
#include <limits.h>
#include <ncurses.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <sys/stat.h>

#define DOT "\xC2\xB7"

static int is_dir(const char *path) {
    struct stat st;
    return stat(path, &st) == 0 && S_ISDIR(st.st_mode);
}

static void add_shortcut(FilePicker *p, const char *label, const char *path) {
    if (p->shortcut_count >= FILE_PICKER_SHORTCUTS || !is_dir(path)) return;
    str_copy(p->shortcut_label[p->shortcut_count], sizeof(p->shortcut_label[0]), label);
    str_copy(p->shortcut_path[p->shortcut_count], sizeof(p->shortcut_path[0]), path);
    p->shortcut_count++;
}

/* Home and the usual media folders, plus the Windows profile under WSL. */
static void build_shortcuts(FilePicker *p) {
    p->shortcut_count = 0;
    char home[512], path[600];
    path_expand_home("~", home, sizeof(home));
    add_shortcut(p, "Home", home);
    const char *folders[] = { "Downloads", "Pictures", "Videos", "Documents" };
    for (int i = 0; i < 4; i++) {
        path_join(path, sizeof(path), home, folders[i]);
        add_shortcut(p, folders[i], path);
    }
    if (!platform_is_wsl()) return;
    DIR *d = opendir("/mnt/c/Users");
    struct dirent *ent;
    while (d && (ent = readdir(d)) != NULL) {
        const char *n = ent->d_name;
        if (n[0] == '.' || !strcasecmp(n, "Public") || !strcasecmp(n, "Default") ||
            !strcasecmp(n, "Default User") || !strcasecmp(n, "All Users") || strchr(n, '.') == n) continue;
        char downloads[600];
        snprintf(downloads, sizeof(downloads), "/mnt/c/Users/%s/Downloads", n);
        if (!is_dir(downloads)) continue;
        char label[32];
        snprintf(label, sizeof(label), "Windows %.18s", n);
        snprintf(path, sizeof(path), "/mnt/c/Users/%s", n);
        add_shortcut(p, label, path);
    }
    if (d) closedir(d);
}

static int matches(const FilePicker *p, const FileEntry *e) {
    return !p->filter[0] || strcasestr(e->name, p->filter) != NULL;
}

/* Entry index for filtered position n (position 0 is ".."), or -1 for "..". */
static int entry_at(const FilePicker *p, int n) {
    if (n == 0) return -1;
    for (int i = 0, k = 1; i < p->count; i++) {
        if (!matches(p, &p->entries[i])) continue;
        if (k++ == n) return i;
    }
    return -2;
}

static int visible(const FilePicker *p) {
    int n = 1;
    for (int i = 0; i < p->count; i++) n += matches(p, &p->entries[i]);
    return n;
}

static void load(FilePicker *p, const char *dir) {
    char real[PATH_MAX];
    if (!realpath(dir, real) || !is_dir(real)) return;
    free(p->entries);
    str_copy(p->dir, sizeof(p->dir), real);
    p->count = dir_listing_read(p->dir, p->show_hidden, &p->entries);
    p->selected = p->count > 0 ? 1 : 0;
    p->scroll = 0;
    p->filter[0] = '\0';
}

void file_picker_init(FilePicker *p) { memset(p, 0, sizeof(*p)); }

void file_picker_dispose(FilePicker *p) {
    free(p->entries);
    p->entries = NULL;
    p->count = 0;
}

void file_picker_open(FilePicker *p, const char *start_dir) {
    build_shortcuts(p);
    char home[512];
    path_expand_home("~", home, sizeof(home));
    load(p, start_dir && *start_dir && is_dir(start_dir) ? start_dir : home);
    p->picked[0] = '\0';
    p->open = 1;
}

void file_picker_close(FilePicker *p) { p->open = 0; }

static void go_up(FilePicker *p) {
    char parent[1024];
    str_copy(parent, sizeof(parent), p->dir);
    char *slash = strrchr(parent, '/');
    if (!slash) return;
    if (slash == parent) slash[1] = '\0';
    else *slash = '\0';
    char came_from[256];
    str_copy(came_from, sizeof(came_from), strrchr(p->dir, '/') ? strrchr(p->dir, '/') + 1 : "");
    load(p, parent);
    for (int n = 1; n < visible(p); n++) {                 /* reselect the folder we left */
        int i = entry_at(p, n);
        if (i >= 0 && !strcmp(p->entries[i].name, came_from)) { p->selected = n; break; }
    }
}

static FilePickerResult choose(FilePicker *p, int n) {
    int i = entry_at(p, n);
    if (i == -1) { go_up(p); return FILE_PICKER_BROWSING; }
    if (i < 0) return FILE_PICKER_BROWSING;
    char path[1300];
    snprintf(path, sizeof(path), "%s%s%s", p->dir, strcmp(p->dir, "/") ? "/" : "", p->entries[i].name);
    if (p->entries[i].is_dir) { load(p, path); return FILE_PICKER_BROWSING; }
    str_copy(p->picked, sizeof(p->picked), path);
    p->open = 0;
    return FILE_PICKER_PICKED;
}

FilePickerResult file_picker_key(FilePicker *p, int is_key, int ch) {
    int n = visible(p);
    if (!is_key && ch == 27) {
        if (p->filter[0]) { p->filter[0] = '\0'; p->selected = 0; return FILE_PICKER_BROWSING; }
        p->open = 0;
        return FILE_PICKER_CANCELLED;
    }
    if (is_key) {
        switch (ch) {
            case KEY_UP:    if (p->selected > 0) p->selected--; break;
            case KEY_DOWN:  if (p->selected < n - 1) p->selected++; break;
            case KEY_PPAGE: p->selected = p->selected > 10 ? p->selected - 10 : 0; break;
            case KEY_NPAGE: p->selected = p->selected + 10 < n ? p->selected + 10 : n - 1; break;
            case KEY_HOME:  p->selected = 0; break;
            case KEY_END:   p->selected = n - 1; break;
            case KEY_LEFT:  go_up(p); break;
            case KEY_RIGHT: case KEY_ENTER: return choose(p, p->selected);
            case KEY_BACKSPACE:
                if (p->filter[0]) p->filter[strlen(p->filter) - 1] = '\0';
                else go_up(p);
                break;
            default: break;
        }
        return FILE_PICKER_BROWSING;
    }
    if (ch == '\n' || ch == '\r') return choose(p, p->selected);
    if (ch == 127 || ch == 8) {
        if (p->filter[0]) p->filter[strlen(p->filter) - 1] = '\0';
        else go_up(p);
        return FILE_PICKER_BROWSING;
    }
    if (ch == '~' && !p->filter[0]) { load(p, p->shortcut_path[0]); return FILE_PICKER_BROWSING; }
    if (ch == '.' && !p->filter[0]) {
        p->show_hidden = !p->show_hidden;
        char dir[1024];
        str_copy(dir, sizeof(dir), p->dir);
        load(p, dir);
        return FILE_PICKER_BROWSING;
    }
    size_t len = strlen(p->filter);
    if (ch >= 32 && ch < 127 && len + 1 < sizeof(p->filter)) {
        p->filter[len] = (char)ch;
        p->filter[len + 1] = '\0';
        p->selected = visible(p) > 1 ? 1 : 0;                 /* first match */
    }
    return FILE_PICKER_BROWSING;
}

void file_picker_wheel(FilePicker *p, int delta) {
    int n = visible(p);
    p->selected += delta;
    if (p->selected < 0) p->selected = 0;
    if (p->selected >= n) p->selected = n - 1;
}

FilePickerResult file_picker_click(FilePicker *p, int y, int x) {
    UiRect r = p->last_rect;
    if (!ui_rect_contains(r, y, x)) return FILE_PICKER_BROWSING;
    if (y == r.y + 2) {                                        /* shortcut chips */
        for (int i = 0; i < p->shortcut_count; i++) {
            if (x >= p->shortcut_x[i] && x < p->shortcut_x[i + 1]) load(p, p->shortcut_path[i]);
        }
        return FILE_PICKER_BROWSING;
    }
    int k = y - r.y;
    if (k < 0 || k >= FILE_PICKER_ROWS || p->row_item[k] < 0) return FILE_PICKER_BROWSING;
    if (p->row_item[k] == p->selected || entry_at(p, p->row_item[k]) < 0 ||
        !p->entries[entry_at(p, p->row_item[k])].is_dir) {
        return choose(p, p->row_item[k]);                      /* files pick on the first click */
    }
    p->selected = p->row_item[k];
    return choose(p, p->selected);
}

static void format_size(long long bytes, char *out, size_t size) {
    if (bytes >= 1024LL * 1024 * 1024) snprintf(out, size, "%.1f GB", bytes / (1024.0 * 1024 * 1024));
    else if (bytes >= 1024 * 1024) snprintf(out, size, "%.1f MB", bytes / (1024.0 * 1024));
    else if (bytes >= 1024) snprintf(out, size, "%lld KB", bytes / 1024);
    else snprintf(out, size, "%lld B", bytes);
}

void file_picker_render(FilePicker *p, UiRect r) {
    p->last_rect = r;
    for (int i = 0; i < FILE_PICKER_ROWS; i++) p->row_item[i] = -1;
    int base = tui_palette_attr(THEME_SLOT_BASE);
    tui_box(r, ICON_ATTACH " Attach a file", tui_palette_attr(THEME_SLOT_BORDER));
    tui_fill((UiRect){ r.y + 1, r.x + 1, r.h - 2, r.w - 2 }, base);

    char where[1100];
    snprintf(where, sizeof(where), " %s%s%s", p->dir, p->filter[0] ? "   \xF0\x9F\x94\x8D " : "", p->filter);
    tui_fill((UiRect){ r.y + 1, r.x + 1, 1, r.w - 2 }, tui_palette_attr(THEME_SLOT_HEADER));
    tui_text(r.y + 1, r.x + 1, r.w - 2, where, tui_palette_attr(THEME_SLOT_HEADER) | ATTR_BOLD);

    int x = r.x + 2;
    for (int i = 0; i < p->shortcut_count; i++) {
        char chip[48];
        snprintf(chip, sizeof(chip), " %s ", p->shortcut_label[i]);
        int active = strcmp(p->dir, p->shortcut_path[i]) == 0;
        p->shortcut_x[i] = x;
        x += tui_text(r.y + 2, x, r.x + r.w - 2 - x, chip,
                      active ? tui_palette_attr(THEME_SLOT_BADGE) | ATTR_BOLD : tui_palette_attr(THEME_SLOT_SIDEBAR_SELECTED)) + 1;
    }
    p->shortcut_x[p->shortcut_count] = x;

    UiRect list = { r.y + 4, r.x + 1, r.h - 6, r.w - 2 };
    int n = visible(p);
    if (p->selected >= n) p->selected = n - 1;
    if (p->selected < p->scroll) p->scroll = p->selected;
    if (p->selected >= p->scroll + list.h) p->scroll = p->selected - list.h + 1;
    for (int k = 0; k < list.h && p->scroll + k < n; k++) {
        int pos = p->scroll + k, i = entry_at(p, pos);
        int attr = pos == p->selected ? tui_palette_attr(THEME_SLOT_SIDEBAR_SELECTED) | ATTR_BOLD : base;
        tui_fill((UiRect){ list.y + k, list.x, 1, list.w }, attr);
        char line[300], size[24] = "";
        if (i == -1) snprintf(line, sizeof(line), "  \xE2\x86\xB0 ..");
        else if (i >= 0 && p->entries[i].is_dir) snprintf(line, sizeof(line), "  \xF0\x9F\x93\x81 %s/", p->entries[i].name);
        else if (i >= 0) {
            snprintf(line, sizeof(line), "  \xF0\x9F\x93\x84 %s", p->entries[i].name);
            format_size(p->entries[i].size, size, sizeof(size));
        }
        int size_cols = size[0] ? tui_text_right(list.y + k, list.x + list.w - 1, 12, size, attr | ATTR_DIM) : 0;
        tui_text(list.y + k, list.x, list.w - size_cols - 2, line, attr);
        if (list.y + k - r.y < FILE_PICKER_ROWS) p->row_item[list.y + k - r.y] = pos;
    }
    tui_text_center(r.y + r.h - 2, r.x, r.w,
                    "Enter pick or open " DOT " \xE2\x86\x90 up " DOT " type to filter " DOT " . hidden files " DOT " Esc cancel",
                    tui_palette_attr(THEME_SLOT_DIM));
}
