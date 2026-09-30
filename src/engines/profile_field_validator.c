#include "engines/profile_field_validator.h"
#include "utilities/str_util.h"

#include <stdio.h>

#define NAME_MAX_CHARS  25
#define ABOUT_MAX_CHARS 139

/* Characters, not bytes: WhatsApp counts what people see. */
static int count_chars(const char *s) {
    int n = 0;
    for (; *s; s++) if (((unsigned char)*s & 0xC0) != 0x80) n++;
    return n;
}

static int blank(const char *s) {
    for (; *s; s++) if (*s != ' ' && *s != '\t' && *s != '\n' && *s != '\r') return 0;
    return 1;
}

int profile_field_max_chars(ProfileField field) {
    switch (field) {
        case PROFILE_FIELD_NAME:  return NAME_MAX_CHARS;
        case PROFILE_FIELD_ABOUT: return ABOUT_MAX_CHARS;
        default:                  return 0;
    }
}

int profile_field_validate(ProfileField field, const char *text, char *why, size_t why_size) {
    if (why && why_size) why[0] = '\0';
    if (field != PROFILE_FIELD_NAME && field != PROFILE_FIELD_ABOUT) {
        if (why) str_copy(why, why_size, "Only the name and about text are text.");
        return -1;
    }
    if (!text) text = "";
    if (field == PROFILE_FIELD_NAME && blank(text)) {
        if (why) str_copy(why, why_size, "Your name cannot be empty.");
        return -1;
    }
    int max = profile_field_max_chars(field);
    int used = count_chars(text);
    if (used > max) {
        if (why) snprintf(why, why_size, "%s can be at most %d characters (this is %d).",
                          field == PROFILE_FIELD_NAME ? "Your name" : "The about text", max, used);
        return -1;
    }
    return 0;
}
