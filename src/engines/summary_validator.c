#include "engines/summary_validator.h"
#include "core/summary.h"

#include <string.h>

static int has_words(const char *text) {
    for (const unsigned char *c = (const unsigned char *)text; *c; c++) if (*c > ' ') return 1;
    return 0;
}

const char *summary_validator_refusal(const Message *message, const char *text) {
    if (!message || message->type != MESSAGE_TYPE_TEXT || !message->text || !message->text[0]) return "Only a text message has a summary";
    if (message->deleted) return "That message was deleted";
    if (!text || !has_words(text)) return "\"text\" is required";
    if (strlen(text) > SUMMARY_MAX_BYTES) return "The summary is too long";
    return NULL;
}

const char *summary_validator_decline_refusal(const Message *message, const char *reason) {
    if (!message || message->type != MESSAGE_TYPE_TEXT || !message->text || !message->text[0]) return "Only a text message is asked for a summary";
    if (message->deleted) return "That message was deleted";
    if (!reason || (strcmp(reason, "too_short") != 0 && strcmp(reason, "nothing_to_say") != 0 && strcmp(reason, "other") != 0))
        return "\"reason\" must be too_short, nothing_to_say or other";
    return NULL;
}

void summary_validator_clean(char *text) {
    if (!text) return;
    char *out = text;
    int space = 1;                                  /* leading spaces are dropped */
    for (const unsigned char *c = (const unsigned char *)text; *c; c++) {
        int blank = *c <= ' ' || *c == 0x7F;
        if (blank) {
            if (!space) *out++ = ' ';
            space = 1;
        } else {
            *out++ = (char)*c;
            space = 0;
        }
    }
    while (out > text && out[-1] == ' ') out--;
    *out = '\0';
}
