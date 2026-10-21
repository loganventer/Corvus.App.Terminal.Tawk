#include "engines/reaction_grouping.h"

#include <string.h>

static int first_of(const Reaction *items, int count, int index) {
    for (int i = 0; i < index; i++) if (strcmp(items[i].emoji, items[index].emoji) == 0) return i;
    return index;
}

static int count_of(const Reaction *items, int count, int index) {
    int n = 0;
    for (int i = 0; i < count; i++) n += strcmp(items[i].emoji, items[index].emoji) == 0;
    return n;
}

/* True when `a` belongs before `b`: a bigger group first, then the group seen first, then the earlier person. */
static int comes_before(const Reaction *items, int count, int a, int b) {
    int ca = count_of(items, count, a), cb = count_of(items, count, b);
    if (ca != cb) return ca > cb;
    int fa = first_of(items, count, a), fb = first_of(items, count, b);
    if (fa != fb) return fa < fb;
    return a < b;
}

void reaction_grouping_sort(Reaction *items, int count) {
    if (count < 2) return;
    Reaction sorted[REACTION_GROUPING_MAX];
    int taken[REACTION_GROUPING_MAX] = { 0 };
    if (count > REACTION_GROUPING_MAX) count = REACTION_GROUPING_MAX;
    for (int out = 0; out < count; out++) {
        int best = -1;
        for (int i = 0; i < count; i++) {
            if (taken[i]) continue;
            if (best < 0 || comes_before(items, count, i, best)) best = i;
        }
        taken[best] = 1;
        sorted[out] = items[best];
    }
    memcpy(items, sorted, sizeof(Reaction) * (size_t)count);
}

int reaction_grouping_groups(const Reaction *items, int count, ReactionGroup *out, int max) {
    int n = 0;
    for (int i = 0; i < count && n < max; i++) {
        int seen = 0;
        for (int g = 0; g < n; g++) {
            if (strcmp(out[g].emoji, items[i].emoji) == 0) { out[g].count++; seen = 1; break; }
        }
        if (seen) continue;
        memcpy(out[n].emoji, items[i].emoji, sizeof(out[n].emoji));
        out[n].count = 1;
        n++;
    }
    /* Bigger groups first; ties keep the order they appeared in. */
    for (int i = 1; i < n; i++) {
        ReactionGroup g = out[i];
        int j = i - 1;
        while (j >= 0 && out[j].count < g.count) { out[j + 1] = out[j]; j--; }
        out[j + 1] = g;
    }
    return n;
}
