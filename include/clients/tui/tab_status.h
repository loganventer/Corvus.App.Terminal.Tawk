#ifndef APP_CLIENTS_TUI_TAB_STATUS_H
#define APP_CLIENTS_TUI_TAB_STATUS_H

#include "contracts/i_terminal_title.h"
#include "core/unread_tally.h"

/* Everything summarised in the terminal tab title. */
typedef struct TabStatus {
    const char       *user_name;
    int               connection;   /* 0 online, 1 connecting/reconnecting, 2 unavailable, 3 not linked */
    int               dnd;
    int               recording;
    int               playing;
    const UnreadTally *tally;
    TerminalProgress  progress;
    const char       *chat_name;    /* the open chat, or "" */
    const char       *activity;     /* what is happening now ("Jan is typing…"), or "" */
    int               busy;         /* spin the leading glyph while something is under way */
    int               typing;       /* the activity is someone typing to you: animate it */
} TabStatus;

#endif
