#include "engines/mention_matcher.h"

#include <ctype.h>
#include <string.h>
#include <strings.h>

/* 2 when a word of `name` starts with `query`, 1 when it contains it, 0 otherwise. */
static int fit(const char *name, const char *query) {
    size_t q = strlen(query);
    if (q == 0) return 2;
    for (const char *p = name; *p; p++) {
        int word_start = p == name || isspace((unsigned char)p[-1]);
        if (word_start && strncasecmp(p, query, q) == 0) return 2;
    }
    return strcasestr(name, query) ? 1 : 0;
}

int mention_matcher_rank(const MentionCandidate *members, int count, const char *query, MentionCandidate *out, int max) {
    int n = 0;
    for (int level = 2; level >= 1 && n < max; level--) {
        for (int i = 0; i < count && n < max; i++) {
            if (fit(members[i].name, query ? query : "") == level) out[n++] = members[i];
        }
    }
    return n;
}
