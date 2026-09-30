#ifndef APP_CORE_EMOJI_H
#define APP_CORE_EMOJI_H

#include "core/emoji_group.h"

/* One emoji with its Unicode name and everyday search words ("hug", "lol"),
 * used for browsing and searching. */
typedef struct Emoji {
    char       glyph[48];
    char       name[96];
    char       keywords[160];   /* space separated, words of the name left out */
    EmojiGroup group;
} Emoji;

#endif
