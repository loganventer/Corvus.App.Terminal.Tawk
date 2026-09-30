#ifndef APP_ENGINES_EMOJI_SHORTCODE_H
#define APP_ENGINES_EMOJI_SHORTCODE_H

#include "contracts/i_emoji_catalog.h"

#define EMOJI_SHORTCODE_MAX 64

/* Emoji for a "(word)" shortcode, best first: "hug" -> 🤗, 🫂; "lol" -> 😂.
 * Every word of `code` must start a word of the emoji's name or keywords; a
 * whole word ranks above a prefix and the name above a keyword, and ties keep
 * the catalog's order (smileys first). Skin tone variants, joined sequences
 * and emoji the system cannot measure are left out. Fills `out` with catalog indexes and returns how many (at most `max`). */
int emoji_shortcode_match(IEmojiCatalog *catalog, const char *code, int *out, int max);

#endif
