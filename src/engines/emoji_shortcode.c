#include "engines/emoji_shortcode.h"
#include "utilities/utf8_text.h"

#include <ctype.h>
#include <string.h>
#include <strings.h>

/* How well one query word fits a list of words: 2 = a whole word, 1 = starts a word, 0 = neither. */
static int word_fit(const char *words, const char *word, size_t len) {
    int best = 0;
    for (const char *p = words; *p;) {
        while (*p && !isalnum((unsigned char)*p)) p++;
        const char *start = p;
        while (*p && isalnum((unsigned char)*p)) p++;
        size_t n = (size_t)(p - start);
        if (n < len || strncasecmp(start, word, len) != 0) continue;
        if (n == len) return 2;
        best = 1;
    }
    return best;
}

/* 0 when some word misses both the name and the keywords; otherwise higher
 * is better. A whole word of the name beats a whole keyword, which beats a
 * prefix of either. */
static int score(const Emoji *e, const char *code) {
    static const int WEIGHT[3][3] = {        /* [name fit][keyword fit] */
        { 0, 2, 5 },
        { 3, 3, 5 },
        { 8, 8, 8 },
    };
    int total = 0, words = 0;
    for (const char *p = code; *p;) {
        while (*p && !isalnum((unsigned char)*p)) p++;
        const char *start = p;
        while (*p && isalnum((unsigned char)*p)) p++;
        size_t len = (size_t)(p - start);
        if (!len) continue;
        int w = WEIGHT[word_fit(e->name, start, len)][word_fit(e->keywords, start, len)];
        if (!w) return 0;
        total += w;
        words++;
    }
    if (!words) return 0;
    if (strcasecmp(e->name, code) == 0) total += 16;                /* the name itself */
    return total;
}

static int usable(const Emoji *e) {
    return e && !strstr(e->glyph, "\xE2\x80\x8D") && !strstr(e->name, "skin tone") &&   /* U+200D joiner */
           utf8_columns(e->glyph) > 0;                               /* newer than the system's tables */
}

int emoji_shortcode_match(IEmojiCatalog *catalog, const char *code, int *out, int max) {
    if (!catalog || !code || !out || max < 1) return 0;
    int scores[EMOJI_SHORTCODE_MAX];
    int count = 0;
    if (max > EMOJI_SHORTCODE_MAX) max = EMOJI_SHORTCODE_MAX;
    int total = catalog->count(catalog);
    for (int i = 0; i < total; i++) {
        const Emoji *e = catalog->at(catalog, i);
        if (!usable(e)) continue;
        int s = score(e, code);
        if (s <= 0) continue;
        int at = count;                                             /* insertion keeps catalog order on ties */
        while (at > 0 && scores[at - 1] < s) at--;
        if (at >= max) continue;
        int last = count < max ? count : max - 1;
        for (int k = last; k > at; k--) { out[k] = out[k - 1]; scores[k] = scores[k - 1]; }
        out[at] = i;
        scores[at] = s;
        if (count < max) count++;
    }
    return count;
}
