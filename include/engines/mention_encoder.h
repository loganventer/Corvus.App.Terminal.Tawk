#ifndef APP_ENGINES_MENTION_ENCODER_H
#define APP_ENGINES_MENTION_ENCODER_H

#include <stddef.h>

#include "core/mention_list.h"
#include "core/mention_pick.h"

/* Turns the "@<name>" of each person picked while typing into the
 * "@<number>" WhatsApp sends, and lists them in `mentions`. People picked
 * whose name is no longer in the text are left out. Writes the text to
 * `out`; returns -1 when it does not fit. */
int mention_encoder_encode(const char *text, const MentionPick *picks, int count, char *out, size_t size, MentionList *mentions);

#endif
