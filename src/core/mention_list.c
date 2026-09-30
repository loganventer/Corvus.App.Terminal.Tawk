#include "core/mention_list.h"
#include "utilities/str_util.h"

#include <stdlib.h>
#include <string.h>

void mention_list_init(MentionList *l) { memset(l, 0, sizeof(*l)); }

int mention_list_add(MentionList *l, const char *jid, const char *user) {
    if (!jid || !*jid) return -1;
    for (int i = 0; i < l->count; i++) if (strcmp(l->items[i].jid, jid) == 0) return 0;
    if (l->count >= MENTION_LIST_MAX) return -1;
    Mention *m = &l->items[l->count++];
    str_copy(m->jid, sizeof(m->jid), jid);
    if (user && *user) {
        str_copy(m->user, sizeof(m->user), user);
    } else {
        const char *at = strchr(jid, '@');
        size_t n = at ? (size_t)(at - jid) : strlen(jid);
        if (n >= sizeof(m->user)) n = sizeof(m->user) - 1;
        memcpy(m->user, jid, n);
        m->user[n] = '\0';
    }
    return 0;
}

const Mention *mention_list_find_user(const MentionList *l, const char *user) {
    for (int i = 0; l && user && i < l->count; i++) if (strcmp(l->items[i].user, user) == 0) return &l->items[i];
    return NULL;
}

char *mention_list_serialize(const MentionList *l) {
    if (!l || l->count == 0) return NULL;
    size_t size = 1;
    for (int i = 0; i < l->count; i++) size += strlen(l->items[i].jid) + strlen(l->items[i].user) + 2;
    char *out = malloc(size);
    if (!out) return NULL;
    out[0] = '\0';
    for (int i = 0; i < l->count; i++) {
        strcat(out, l->items[i].jid);
        strcat(out, "\t");
        strcat(out, l->items[i].user);
        strcat(out, "\n");
    }
    return out;
}

void mention_list_parse(MentionList *l, const char *text) {
    mention_list_init(l);
    for (const char *line = text; line && *line;) {
        const char *end = strchr(line, '\n');
        size_t len = end ? (size_t)(end - line) : strlen(line);
        const char *tab = memchr(line, '\t', len);
        if (tab) {
            char jid[128], user[64];
            size_t jl = (size_t)(tab - line), ul = len - jl - 1;
            if (jl < sizeof(jid) && ul < sizeof(user)) {
                memcpy(jid, line, jl);
                jid[jl] = '\0';
                memcpy(user, tab + 1, ul);
                user[ul] = '\0';
                mention_list_add(l, jid, user);
            }
        }
        line = end ? end + 1 : NULL;
    }
}
