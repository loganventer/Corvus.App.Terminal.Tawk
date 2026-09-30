#include "engines/chat_visibility.h"

#include <string.h>

int chat_visibility_hidden(const char *jid) {
    static const char SUFFIX[] = "@broadcast";
    if (!jid) return 0;
    size_t len = strlen(jid), n = sizeof(SUFFIX) - 1;
    return len >= n && strcmp(jid + len - n, SUFFIX) == 0;
}
