#ifndef APP_ENGINES_EMOTICON_CONVERTER_H
#define APP_ENGINES_EMOTICON_CONVERTER_H

/* Text emoticons as emoji, like other chat apps: ":)" -> 🙂, "<3" -> ❤️.
 * Only a whole word matches, so ":/" inside "http://" is left alone.
 * Returns the emoji (UTF-8), or NULL when `word` is not an emoticon. */
const char *emoticon_to_emoji(const char *word);

#endif
