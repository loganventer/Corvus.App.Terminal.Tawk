#include "resource_access/json_theme_repository.h"
#include "utilities/color_util.h"
#include "utilities/log.h"
#include "utilities/path_util.h"
#include "utilities/str_util.h"

#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>

#include "cJSON.h"

#define MAX_THEME_FILE_BYTES (64 * 1024)

typedef struct ThemeRepo {
    char   bundled_dir[512];
    char   user_dir[512];
    Theme *themes;
    int    count;
    int    cap;
} ThemeRepo;

static ThemeRepo *ctx_of(IThemeRepository *self) { return (ThemeRepo *)self->ctx; }

static char *read_small_file(const char *path) {
    FILE *f = fopen(path, "rb");
    if (!f) return NULL;
    char *buf = malloc(MAX_THEME_FILE_BYTES + 1);
    size_t n = buf ? fread(buf, 1, MAX_THEME_FILE_BYTES + 1, f) : 0;
    fclose(f);
    if (!buf || n > MAX_THEME_FILE_BYTES) { free(buf); return NULL; }
    buf[n] = '\0';
    return buf;
}

static short color_field(const cJSON *obj, const char *key, short fallback) {
    const cJSON *item = cJSON_GetObjectItemCaseSensitive(obj, key);
    if (cJSON_IsNumber(item) && item->valueint >= -1 && item->valueint <= 255) return (short)item->valueint;
    if (cJSON_IsString(item)) {
        short c = color_parse(item->valuestring);
        if (c != -2) return c;
    }
    return fallback;
}

/* Missing slots inherit sensible neighbours so small theme files still work. */
static const ThemeSlot INHERIT[THEME_SLOT_COUNT] = {
    THEME_SLOT_BASE, THEME_SLOT_BASE, THEME_SLOT_BASE, THEME_SLOT_HEADER, THEME_SLOT_ACCENT,
    THEME_SLOT_BASE, THEME_SLOT_HEADER, THEME_SLOT_SIDEBAR_SELECTED, THEME_SLOT_BUBBLE_THEM,
    THEME_SLOT_DIM, THEME_SLOT_SIDEBAR_SELECTED, THEME_SLOT_SIDEBAR_SELECTED, THEME_SLOT_BASE,
    THEME_SLOT_HEADER, THEME_SLOT_HEADER, THEME_SLOT_BADGE, THEME_SLOT_ACCENT, THEME_SLOT_BASE,
    THEME_SLOT_DIM, THEME_SLOT_ACCENT
};

static int parse_theme(const char *json, const char *fallback_id, Theme *out) {
    cJSON *root = cJSON_Parse(json);
    if (!root) return -1;
    const cJSON *colors = cJSON_GetObjectItemCaseSensitive(root, "colors");
    const cJSON *id = cJSON_GetObjectItemCaseSensitive(root, "id");
    const cJSON *name = cJSON_GetObjectItemCaseSensitive(root, "name");
    const cJSON *desc = cJSON_GetObjectItemCaseSensitive(root, "description");
    if (!cJSON_IsObject(colors)) { cJSON_Delete(root); return -1; }

    Theme t;
    theme_set_default(&t);
    str_copy(t.id, sizeof(t.id), cJSON_IsString(id) ? id->valuestring : fallback_id);
    str_copy(t.name, sizeof(t.name), cJSON_IsString(name) ? name->valuestring : t.id);
    str_copy(t.description, sizeof(t.description), cJSON_IsString(desc) ? desc->valuestring : "");
    str_strip_controls(t.id);
    str_strip_controls(t.name);
    str_strip_controls(t.description);

    int present[THEME_SLOT_COUNT] = {0};
    for (int s = 0; s < THEME_SLOT_COUNT; s++) {
        const cJSON *slot = cJSON_GetObjectItemCaseSensitive(colors, theme_slot_key((ThemeSlot)s));
        if (!cJSON_IsObject(slot)) continue;
        t.colors[s].fg = color_field(slot, "fg", t.colors[s].fg);
        t.colors[s].bg = color_field(slot, "bg", t.colors[s].bg);
        present[s] = 1;
    }
    for (int s = 1; s < THEME_SLOT_COUNT; s++) {
        if (!present[s]) t.colors[s] = t.colors[INHERIT[s]];
    }
    cJSON_Delete(root);
    *out = t;
    return 0;
}

static void upsert(ThemeRepo *r, const Theme *t) {
    for (int i = 0; i < r->count; i++) {
        if (strcmp(r->themes[i].id, t->id) == 0) { r->themes[i] = *t; return; }
    }
    if (r->count == r->cap) {
        int ncap = r->cap ? r->cap * 2 : 64;
        Theme *grown = realloc(r->themes, (size_t)ncap * sizeof(Theme));
        if (!grown) return;
        r->themes = grown;
        r->cap = ncap;
    }
    r->themes[r->count++] = *t;
}

static void load_dir(ThemeRepo *r, const char *dir) {
    DIR *d = dir[0] ? opendir(dir) : NULL;
    if (!d) return;
    struct dirent *ent;
    while ((ent = readdir(d)) != NULL) {
        size_t n = strlen(ent->d_name);
        if (n < 6 || n > 100 || strcmp(ent->d_name + n - 5, ".json") != 0 || ent->d_name[0] == '.') continue;
        char path[1024], id[128];
        path_join(path, sizeof(path), dir, ent->d_name);
        if (!path_is_regular_file(path)) continue;
        str_copy(id, sizeof(id), ent->d_name);
        id[n - 5] = '\0';
        char *json = read_small_file(path);
        Theme t;
        if (json && parse_theme(json, id, &t) == 0) upsert(r, &t);
        else LOG_WARN("theme %s is not valid and was skipped", path);
        free(json);
    }
    closedir(d);
}

static int by_name(const void *a, const void *b) {
    return strcasecmp(((const Theme *)a)->name, ((const Theme *)b)->name);
}

static int repo_reload(IThemeRepository *self) {
    ThemeRepo *r = ctx_of(self);
    r->count = 0;
    Theme fallback;
    theme_set_default(&fallback);
    upsert(r, &fallback);
    load_dir(r, r->bundled_dir);
    load_dir(r, r->user_dir);
    qsort(r->themes, (size_t)r->count, sizeof(Theme), by_name);
    LOG_INFO("loaded %d theme(s)", r->count);
    return r->count;
}

static int repo_count(IThemeRepository *self) { return ctx_of(self)->count; }

static const Theme *repo_at(IThemeRepository *self, int index) {
    ThemeRepo *r = ctx_of(self);
    return (index >= 0 && index < r->count) ? &r->themes[index] : &r->themes[0];
}

static int repo_index_of(IThemeRepository *self, const char *id) {
    ThemeRepo *r = ctx_of(self);
    for (int i = 0; id && i < r->count; i++) if (strcmp(r->themes[i].id, id) == 0) return i;
    return -1;
}

static void repo_destroy(IThemeRepository *self) {
    if (!self) return;
    free(ctx_of(self)->themes);
    free(self->ctx);
    free(self);
}

IThemeRepository *json_theme_repository_create(const char *bundled_dir, const char *user_dir) {
    IThemeRepository *repo = calloc(1, sizeof(*repo));
    ThemeRepo *r = calloc(1, sizeof(*r));
    if (!repo || !r) { free(repo); free(r); return NULL; }
    str_copy(r->bundled_dir, sizeof(r->bundled_dir), bundled_dir ? bundled_dir : "");
    str_copy(r->user_dir, sizeof(r->user_dir), user_dir ? user_dir : "");
    repo->ctx = r;
    repo->count = repo_count;
    repo->at = repo_at;
    repo->index_of = repo_index_of;
    repo->reload = repo_reload;
    repo->destroy = repo_destroy;
    repo_reload(repo);
    return repo;
}
