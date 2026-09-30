#include "resource_access/tsv_emoji_catalog.h"
#include "utilities/log.h"
#include "utilities/str_util.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_EMOJI 8192

typedef struct Catalog {
    Emoji *items;
    int    count;
} Catalog;

static Catalog *ctx_of(IEmojiCatalog *self) { return (Catalog *)self->ctx; }

static int cat_count(IEmojiCatalog *self) { return ctx_of(self)->count; }

static const Emoji *cat_at(IEmojiCatalog *self, int i) {
    Catalog *c = ctx_of(self);
    return (i >= 0 && i < c->count) ? &c->items[i] : NULL;
}

static int cat_find(IEmojiCatalog *self, const char *glyph) {
    Catalog *c = ctx_of(self);
    for (int i = 0; glyph && i < c->count; i++) if (strcmp(c->items[i].glyph, glyph) == 0) return i;
    return -1;
}

static void cat_destroy(IEmojiCatalog *self) {
    if (!self) return;
    free(ctx_of(self)->items);
    free(self->ctx);
    free(self);
}

static void load(Catalog *c, const char *path) {
    FILE *f = path ? fopen(path, "r") : NULL;
    if (!f) { LOG_WARN("emoji list not found at %s", path ? path : "(none)"); return; }
    c->items = calloc(MAX_EMOJI, sizeof(Emoji));
    char line[512];
    while (c->items && c->count < MAX_EMOJI && fgets(line, sizeof(line), f)) {
        if (line[0] == '#' || line[0] == '\n') continue;
        char *glyph = strtok(line, "\t");
        char *group = strtok(NULL, "\t");
        char *name = strtok(NULL, "\t\n");
        char *keywords = strtok(NULL, "\t\n");   /* optional */
        if (!glyph || !group || !name) continue;
        int g = atoi(group);
        if (g < 0 || g >= EMOJI_GROUP_COUNT) continue;
        Emoji *e = &c->items[c->count++];
        str_copy(e->glyph, sizeof(e->glyph), glyph);
        str_copy(e->name, sizeof(e->name), name);
        str_copy(e->keywords, sizeof(e->keywords), keywords ? keywords : "");
        str_strip_controls(e->keywords);
        str_strip_controls(e->glyph);
        str_strip_controls(e->name);
        e->group = (EmojiGroup)g;
    }
    fclose(f);
    LOG_INFO("loaded %d emoji", c->count);
}

IEmojiCatalog *tsv_emoji_catalog_create(const char *path) {
    IEmojiCatalog *s = calloc(1, sizeof(*s));
    Catalog *c = calloc(1, sizeof(*c));
    if (!s || !c) { free(s); free(c); return NULL; }
    load(c, path);
    s->ctx = c;
    s->count = cat_count;
    s->at = cat_at;
    s->find = cat_find;
    s->destroy = cat_destroy;
    return s;
}
