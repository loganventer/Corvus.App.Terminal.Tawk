#ifndef APP_CORE_WRITE_KIND_H
#define APP_CORE_WRITE_KIND_H

/* How much a write asked for over the control socket can change. */
typedef enum WriteKind {
    WRITE_KIND_SEND = 0,        /* sends, reacts, schedules, drafts: access send */
    WRITE_KIND_MANAGE,          /* changes tawk or WhatsApp settings: access manage */
    WRITE_KIND_DESTRUCTIVE      /* deletes or blocks: access manage, two steps, always asked */
} WriteKind;

#endif
