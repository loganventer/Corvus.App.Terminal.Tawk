#include "engines/chat_match.h"

#include <string.h>
#include <strings.h>

int chat_match_filter(const Chat *c, const char *filter) {
    if (!filter || !filter[0]) return 1;
    return strcasestr(c->name, filter) != NULL || strstr(c->jid, filter) != NULL;
}

int chat_match_searchable(const Chat *c, int in_locked_folder) {
    return c->is_locked <= 0 || in_locked_folder;
}
