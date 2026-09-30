#ifndef APP_CORE_CHAT_RESOLUTION_H
#define APP_CORE_CHAT_RESOLUTION_H

/* How a chat named by a control socket client was found. */
typedef enum ChatResolution {
    CHAT_RESOLUTION_FOUND = 0,
    CHAT_RESOLUTION_NOT_FOUND,
    CHAT_RESOLUTION_AMBIGUOUS
} ChatResolution;

#endif
