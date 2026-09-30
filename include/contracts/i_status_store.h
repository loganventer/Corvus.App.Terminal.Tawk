#ifndef APP_CONTRACTS_I_STATUS_STORE_H
#define APP_CONTRACTS_I_STATUS_STORE_H

#include <stdint.h>

#include "core/status_author.h"
#include "core/status_update.h"

/* Statuses received or posted, kept until they expire. */
typedef struct IStatusStore {
    void *ctx;
    /* Keeps the first copy of an id; later copies only fill in what was missing. */
    int  (*save)(struct IStatusStore *self, const StatusUpdate *update);
    int  (*remove)(struct IStatusStore *self, const char *id);
    int  (*get)(struct IStatusStore *self, const char *id, StatusUpdate *out);
    int  (*set_media_path)(struct IStatusStore *self, const char *id, const char *path);
    int  (*mark_viewed)(struct IStatusStore *self, const char *id);
    /* Authors with statuses posted after `since` and no later than `until`,
     * newest first. Returns how many were written. */
    int  (*authors)(struct IStatusStore *self, int64_t since, int64_t until, StatusAuthor *out, int max);
    /* One author's statuses in the same window, oldest first; the caller disposes each. */
    int  (*updates_by)(struct IStatusStore *self, const char *jid, int64_t since, int64_t until, StatusUpdate *out, int max);
    /* Forgets statuses older than `before`; returns the media paths through `on_media` so they can be deleted. */
    int  (*prune)(struct IStatusStore *self, int64_t before, void (*on_media)(void *ctx, const char *path), void *ctx);
    void (*destroy)(struct IStatusStore *self);
} IStatusStore;

#endif
