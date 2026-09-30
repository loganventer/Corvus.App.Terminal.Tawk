#ifndef APP_CORE_MENTION_PICK_H
#define APP_CORE_MENTION_PICK_H

/* Someone picked from the mention list while typing: the text holds
 * "@<name>" until the message is sent. */
typedef struct MentionPick {
    char jid[128];
    char name[128];
} MentionPick;

#endif
