#ifndef APP_CONTRACTS_I_CHAT_EXPORTER_H
#define APP_CONTRACTS_I_CHAT_EXPORTER_H

#include <stddef.h>

#include "core/message.h"

/* Writes a chat's messages somewhere readable outside tawk. */
typedef struct IChatExporter {
    void *ctx;
    /* Exports `messages` (oldest first) of the chat called `chat_name` into
     * `dir`; with_media also copies the downloaded files. `sender_name`
     * names a sender. The created file or folder goes to `out`. */
    int  (*export_chat)(struct IChatExporter *self, const char *chat_name, const Message *messages, int count,
                        const char *(*sender_name)(void *ctx, const Message *message), void *names_ctx,
                        const char *dir, int with_media, char *out, size_t size);
    void (*destroy)(struct IChatExporter *self);
} IChatExporter;

#endif
