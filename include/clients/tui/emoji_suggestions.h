#ifndef APP_CLIENTS_TUI_EMOJI_SUGGESTIONS_H
#define APP_CLIENTS_TUI_EMOJI_SUGGESTIONS_H

#include "clients/tui/ui_rect.h"
#include "contracts/i_emoji_catalog.h"
#include "engines/emoji_shortcode.h"

/* The emoji a "(word" could mean, in a strip above the input that narrows
 * as the word is typed. ←→ or Tab choose, Enter or a click replaces the
 * shortcode, Esc keeps the text as typed. The strip scrolls sideways. */
typedef struct EmojiSuggestions {
    int    count;                               /* 0 = closed */
    int    matches[EMOJI_SHORTCODE_MAX];        /* catalog indexes */
    int    selected;
    int    start;                               /* the shortcode in the input: [start, end) */
    int    end;
    int    first;                               /* first one shown */
    int    dismissed;                           /* 1 + start of a shortcode Esc turned down, or 0 */
    UiRect cells[EMOJI_SHORTCODE_MAX];          /* where each was drawn, for clicks */
} EmojiSuggestions;

void emoji_suggestions_open(EmojiSuggestions *s, const int *matches, int count, int start, int end);
void emoji_suggestions_close(EmojiSuggestions *s);
/* Esc: close, and stay closed while the same shortcode is typed. */
void emoji_suggestions_dismiss(EmojiSuggestions *s);
void emoji_suggestions_move(EmojiSuggestions *s, int delta);
/* The selected emoji's glyph, or NULL when closed. */
const char *emoji_suggestions_glyph(const EmojiSuggestions *s, IEmojiCatalog *catalog);
/* The suggestion under (y, x), or -1. */
int  emoji_suggestions_hit(const EmojiSuggestions *s, int y, int x);
/* Draws the strip so its bottom sits just above `anchor` (the input). */
void emoji_suggestions_render(EmojiSuggestions *s, IEmojiCatalog *catalog, UiRect anchor);

#endif
