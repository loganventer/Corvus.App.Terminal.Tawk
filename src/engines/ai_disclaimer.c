#include "engines/ai_disclaimer.h"

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

char *ai_disclaimer_append(const char *text, const char *disclaimer) {
    if (!text || !disclaimer) return NULL;
    while (isspace((unsigned char)*disclaimer)) disclaimer++;
    size_t d = strlen(disclaimer), t = strlen(text);
    while (d > 0 && isspace((unsigned char)disclaimer[d - 1])) d--;
    while (t > 0 && isspace((unsigned char)text[t - 1])) t--;
    if (d == 0) return NULL;
    if (t >= d && strncmp(text + t - d, disclaimer, d) == 0) return NULL;      /* already there */
    char *out = malloc(t + d + 3);
    if (!out) return NULL;
    snprintf(out, t + d + 3, "%.*s%s%.*s", (int)t, text, t ? "\n\n" : "", (int)d, disclaimer);
    return out;
}
