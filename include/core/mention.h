#ifndef APP_CORE_MENTION_H
#define APP_CORE_MENTION_H

/* One person mentioned in a message. WhatsApp writes "@<user>" in the text
 * and lists the JID beside it; `user` is those digits (a phone number, or
 * the hidden id in groups that address members by LID). */
typedef struct Mention {
    char jid[128];
    char user[64];
} Mention;

#endif
