#ifndef APP_CONTRACTS_I_MEDIA_OPENER_H
#define APP_CONTRACTS_I_MEDIA_OPENER_H

#include "core/message_type.h"

typedef struct IMediaOpener {
    void *ctx;
    /* Opens the file in the platform's viewer without blocking. */
    int  (*open)(struct IMediaOpener *self, const char *path, MessageType type);
    void (*destroy)(struct IMediaOpener *self);
} IMediaOpener;

#endif
