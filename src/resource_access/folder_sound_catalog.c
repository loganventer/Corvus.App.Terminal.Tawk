#include "resource_access/folder_sound_catalog.h"
#include "engines/sound_name.h"
#include "utilities/path_util.h"
#include "utilities/str_util.h"

#include <dirent.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>

typedef struct SoundCatalog {
    char         bundled_dir[512];
    char         user_dir[512];
    SoundOption *items;
    int          count;
    int          cap;
} SoundCatalog;

static SoundCatalog *ctx_of(ISoundCatalog *self) { return (SoundCatalog *)self->ctx; }

static int find_id(const SoundCatalog *c, const char *id) {
    for (int i = 0; i < c->count; i++) {
        if (strcmp(c->items[i].id, id) == 0) return i;
    }
    return -1;
}

static void add(SoundCatalog *c, const char *dir, const char *file_name) {
    char path[sizeof(c->items[0].path)];
    if (strlen(dir) + strlen(file_name) + 2 > sizeof(path)) return;
    path_join(path, sizeof(path), dir, file_name);
    if (!path_is_regular_file(path)) return;
    SoundOption option;
    memset(&option, 0, sizeof(option));
    sound_name_label(path, option.id, sizeof(option.id));
    if (!option.id[0] || strlen(file_name) >= sizeof(option.id) + 4) return;
    str_copy(option.path, sizeof(option.path), path);
    int at = find_id(c, option.id);
    if (at >= 0) { c->items[at] = option; return; }
    if (c->count == c->cap) {
        int cap = c->cap ? c->cap * 2 : 16;
        SoundOption *grown = realloc(c->items, (size_t)cap * sizeof(*grown));
        if (!grown) return;
        c->items = grown;
        c->cap = cap;
    }
    c->items[c->count++] = option;
}

static void scan(SoundCatalog *c, const char *dir) {
    if (!dir || !dir[0]) return;
    DIR *d = opendir(dir);
    if (!d) return;
    for (struct dirent *e = readdir(d); e; e = readdir(d)) {
        if (sound_name_is_playable(e->d_name)) add(c, dir, e->d_name);
    }
    closedir(d);
}

static int by_id(const void *a, const void *b) {
    return strcasecmp(((const SoundOption *)a)->id, ((const SoundOption *)b)->id);
}

static int catalog_reload(ISoundCatalog *self) {
    SoundCatalog *c = ctx_of(self);
    c->count = 0;
    scan(c, c->bundled_dir);
    scan(c, c->user_dir);
    qsort(c->items, (size_t)c->count, sizeof(*c->items), by_id);
    return 0;
}

static int catalog_count(ISoundCatalog *self) { return ctx_of(self)->count; }

static const SoundOption *catalog_at(ISoundCatalog *self, int index) {
    SoundCatalog *c = ctx_of(self);
    return index >= 0 && index < c->count ? &c->items[index] : NULL;
}

static int catalog_index_of_path(ISoundCatalog *self, const char *path) {
    SoundCatalog *c = ctx_of(self);
    for (int i = 0; path && i < c->count; i++) {
        if (strcmp(c->items[i].path, path) == 0) return i;
    }
    return -1;
}

static int catalog_index_of_id(ISoundCatalog *self, const char *id) {
    return id ? find_id(ctx_of(self), id) : -1;
}

static void catalog_destroy(ISoundCatalog *self) {
    free(ctx_of(self)->items);
    free(self->ctx);
    free(self);
}

ISoundCatalog *folder_sound_catalog_create(const char *bundled_dir, const char *user_dir) {
    ISoundCatalog *self = calloc(1, sizeof(*self));
    SoundCatalog *c = calloc(1, sizeof(*c));
    if (!self || !c) { free(self); free(c); return NULL; }
    str_copy(c->bundled_dir, sizeof(c->bundled_dir), bundled_dir ? bundled_dir : "");
    str_copy(c->user_dir, sizeof(c->user_dir), user_dir ? user_dir : "");
    self->ctx = c;
    self->count = catalog_count;
    self->at = catalog_at;
    self->index_of_path = catalog_index_of_path;
    self->index_of_id = catalog_index_of_id;
    self->reload = catalog_reload;
    self->destroy = catalog_destroy;
    catalog_reload(self);
    return self;
}
