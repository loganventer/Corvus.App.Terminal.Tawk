#include "engines/mention_encoder.h"
#include "utilities/str_util.h"

#include <stdlib.h>
#include <string.h>

/* The digits of a JID: "27821234567" from "27821234567@s.whatsapp.net". */
static void user_of(const char *jid, char *out, size_t size) {
    const char *at = strchr(jid, '@');
    size_t n = at ? (size_t)(at - jid) : strlen(jid);
    if (n >= size) n = size - 1;
    memcpy(out, jid, n);
    out[n] = '\0';
}

static int longer_name_first(const void *a, const void *b) {
    return (int)strlen(((const MentionPick *)b)->name) - (int)strlen(((const MentionPick *)a)->name);
}

int mention_encoder_encode(const char *text, const MentionPick *picks, int count, char *out, size_t size, MentionList *mentions) {
    mention_list_init(mentions);
    if (!text || size == 0) return -1;
    /* Longer names first, so "@Jan de Wet" is not taken for "@Jan". */
    MentionPick sorted[MENTION_LIST_MAX];
    int n = count < MENTION_LIST_MAX ? count : MENTION_LIST_MAX;
    memcpy(sorted, picks, sizeof(MentionPick) * (size_t)n);
    qsort(sorted, (size_t)n, sizeof(MentionPick), longer_name_first);

    size_t used = 0;
    for (const char *p = text; *p;) {
        int matched = 0;
        if (*p == '@') {
            for (int i = 0; i < n; i++) {
                size_t len = strlen(sorted[i].name);
                if (len == 0 || strncmp(p + 1, sorted[i].name, len) != 0) continue;
                char user[64];
                user_of(sorted[i].jid, user, sizeof(user));
                size_t add = 1 + strlen(user);
                if (used + add + 1 > size) return -1;
                out[used++] = '@';
                memcpy(out + used, user, strlen(user));
                used += strlen(user);
                mention_list_add(mentions, sorted[i].jid, user);
                p += 1 + len;
                matched = 1;
                break;
            }
        }
        if (matched) continue;
        if (used + 2 > size) return -1;
        out[used++] = *p++;
    }
    out[used] = '\0';
    return 0;
}
