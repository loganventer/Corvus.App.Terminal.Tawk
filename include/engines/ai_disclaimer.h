#ifndef APP_ENGINES_AI_DISCLAIMER_H
#define APP_ENGINES_AI_DISCLAIMER_H

/* The line added under a message an AI wrote, so whoever gets it knows.
 * Returns `text` with a blank line and `disclaimer` after it, newly
 * allocated (the caller frees it), or NULL when there is nothing to add:
 * no disclaimer, or the text already ends with it. */
char *ai_disclaimer_append(const char *text, const char *disclaimer);

#endif
