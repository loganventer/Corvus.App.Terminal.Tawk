#ifndef APP_CONTRACTS_I_EMOJI_CATALOG_H
#define APP_CONTRACTS_I_EMOJI_CATALOG_H

#include "core/emoji.h"

/* The full emoji set, in Unicode order. */
typedef struct IEmojiCatalog {
    void *ctx;
    int          (*count)(struct IEmojiCatalog *self);
    const Emoji *(*at)(struct IEmojiCatalog *self, int index);
    /* Index of the emoji with this glyph, or -1. */
    int          (*find)(struct IEmojiCatalog *self, const char *glyph);
    void         (*destroy)(struct IEmojiCatalog *self);
} IEmojiCatalog;

#endif
