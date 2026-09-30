#ifndef APP_ENGINES_CHAT_REFERENCE_RESOLVER_H
#define APP_ENGINES_CHAT_REFERENCE_RESOLVER_H

#include "core/chat.h"
#include "core/chat_resolution.h"
#include "core/settings.h"

/* Finds the one chat a control socket client means by `ref`: a JID, a
 * phone number, a name (any case), or the start of a word in a name.
 * Only chats the automation policy allows are considered. On FOUND,
 * *found is its index; on AMBIGUOUS, the indexes of the chats that
 * matched go to `candidates` (up to `max`). */
ChatResolution chat_reference_resolve(const Chat *chats, int count, const Settings *settings, const char *ref,
                                      int *found, int *candidates, int max, int *candidate_count);

#endif
