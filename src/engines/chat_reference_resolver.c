#include "engines/chat_reference_resolver.h"
#include "engines/automation_policy.h"

#include <ctype.h>
#include <string.h>
#include <strings.h>

typedef int (*Matcher)(const Chat *chat, const char *ref);

static int by_jid(const Chat *c, const char *ref) { return strcmp(c->jid, ref) == 0; }

static int by_number(const Chat *c, const char *ref) {
    char want[64], have[64];
    size_t k = 0;
    for (const char *p = ref; *p && k + 1 < sizeof(want); p++) {
        if (isdigit((unsigned char)*p)) want[k++] = *p;
        else if (*p != '+' && *p != ' ' && *p != '-') return 0;
    }
    want[k] = '\0';
    k = 0;
    for (const char *p = c->jid; *p && *p != '@' && k + 1 < sizeof(have); p++) have[k++] = *p;
    have[k] = '\0';
    return strlen(want) >= 6 && strcmp(want, have) == 0;
}

static int by_name(const Chat *c, const char *ref) { return strcasecmp(c->name, ref) == 0; }

static int by_word_start(const Chat *c, const char *ref) {
    size_t n = strlen(ref);
    for (const char *p = c->name; *p; p++) {
        int word_start = p == c->name || isspace((unsigned char)p[-1]) || p[-1] == '(' || p[-1] == '-';
        if (word_start && strncasecmp(p, ref, n) == 0) return 1;
    }
    return 0;
}

ChatResolution chat_reference_resolve(const Chat *chats, int count, const Settings *s, const char *ref,
                                      int *found, int *candidates, int max, int *candidate_count) {
    static const Matcher MATCHERS[] = { by_jid, by_number, by_name, by_word_start };
    if (candidate_count) *candidate_count = 0;
    if (!ref || !*ref) return CHAT_RESOLUTION_NOT_FOUND;
    for (size_t m = 0; m < sizeof(MATCHERS) / sizeof(MATCHERS[0]); m++) {
        int hits = 0, first = -1;
        for (int i = 0; i < count; i++) {
            if (!automation_policy_chat_allowed(s, &chats[i]) || !MATCHERS[m](&chats[i], ref)) continue;
            if (hits == 0) first = i;
            if (candidates && candidate_count && *candidate_count < max) candidates[(*candidate_count)++] = i;
            hits++;
        }
        if (hits == 1) {
            *found = first;
            if (candidate_count) *candidate_count = 0;
            return CHAT_RESOLUTION_FOUND;
        }
        if (hits > 1) return CHAT_RESOLUTION_AMBIGUOUS;
    }
    return CHAT_RESOLUTION_NOT_FOUND;
}
