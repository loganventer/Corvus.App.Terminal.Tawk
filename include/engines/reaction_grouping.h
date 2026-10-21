#ifndef APP_ENGINES_REACTION_GROUPING_H
#define APP_ENGINES_REACTION_GROUPING_H

#include "core/reaction.h"

/* The most reactions the list handles; the rest are left where they are. */
#define REACTION_GROUPING_MAX 64

/* One emoji and how many people used it. */
typedef struct ReactionGroup {
    char emoji[32];
    int  count;
} ReactionGroup;

/* Puts reactions in the order the "who reacted" list shows them: the emoji
 * most people used first, ties kept in the order they first appeared, and
 * people sharing an emoji kept in the order given. Stable. */
void reaction_grouping_sort(Reaction *items, int count);

/* The emoji used and how many used each, in the same order as the sort.
 * Returns how many groups were written, at most `max`. */
int  reaction_grouping_groups(const Reaction *items, int count, ReactionGroup *out, int max);

#endif
