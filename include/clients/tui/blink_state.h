#ifndef APP_CLIENTS_TUI_BLINK_STATE_H
#define APP_CLIENTS_TUI_BLINK_STATE_H

#include <stdint.h>

/* Which chat is blinking for a new message, and until when. */
typedef struct BlinkState {
    char    jid[128];
    int64_t until_ms;
} BlinkState;

/* True during the visible half of the blink cycle. */
int blink_state_on(const BlinkState *blink, const char *jid, int64_t now_ms);

#endif
