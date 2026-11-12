#ifndef APP_CORE_UNLOCK_PURPOSE_H
#define APP_CORE_UNLOCK_PURPOSE_H

/* Why tawk asks for Touch ID or the lock password. */
typedef enum {
    UNLOCK_FOR_OPEN = 0,          /* open a private chat */
    UNLOCK_FOR_MAKE_PUBLIC,       /* turn a private chat back into an ordinary one */
    UNLOCK_FOR_EXPORT,            /* write a private chat to a file */
    UNLOCK_FOR_PASSWORD_CHANGE,   /* set a new lock password */
    UNLOCK_PURPOSE_COUNT
} UnlockPurpose;

/* The end of the Touch ID sentence, such as "open a private chat". It never
 * names the chat, so nothing private reaches the system sheet. */
const char *unlock_purpose_reason(UnlockPurpose purpose);

#endif
