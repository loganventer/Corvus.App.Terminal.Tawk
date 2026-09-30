#ifndef APP_ENGINES_MENTION_MATCHER_H
#define APP_ENGINES_MENTION_MATCHER_H

#include "core/mention_candidate.h"

/* The members that fit what was typed after "@", best first: names where a
 * word starts with it, then names that contain it, ignoring case. An empty
 * query lists everyone in the order given. Returns how many were written. */
int mention_matcher_rank(const MentionCandidate *members, int count, const char *query, MentionCandidate *out, int max);

#endif
