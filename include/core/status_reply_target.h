#ifndef APP_CORE_STATUS_REPLY_TARGET_H
#define APP_CORE_STATUS_REPLY_TARGET_H

/* The status being answered: whose it is, and what the reply quotes. */
typedef struct StatusReplyTarget {
    char status_id[64];
    char author_jid[128];
    char preview[256];    /* its words, or "📷 Photo" and the like */
} StatusReplyTarget;

#endif
