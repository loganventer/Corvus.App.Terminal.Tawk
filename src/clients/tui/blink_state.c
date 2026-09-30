#include "clients/tui/blink_state.h"

#include <string.h>

int blink_state_on(const BlinkState *b, const char *jid, int64_t now_ms) {
    if (now_ms >= b->until_ms) return 0;
    if (jid && strcmp(b->jid, jid) != 0) return 0;
    return (now_ms / 500) % 2 == 0;
}
