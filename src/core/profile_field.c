#include "core/profile_field.h"

#include <string.h>

static const char *const NAMES[PROFILE_FIELD_COUNT]  = { "name", "about", "picture" };
static const char *const LABELS[PROFILE_FIELD_COUNT] = { "Name", "About", "Photo" };

static int valid(ProfileField field) { return field >= 0 && field < PROFILE_FIELD_COUNT; }

const char *profile_field_name(ProfileField field) { return valid(field) ? NAMES[field] : ""; }
const char *profile_field_label(ProfileField field) { return valid(field) ? LABELS[field] : ""; }

ProfileField profile_field_parse(const char *name) {
    for (int i = 0; name && i < PROFILE_FIELD_COUNT; i++) {
        if (strcmp(name, NAMES[i]) == 0) return (ProfileField)i;
    }
    return PROFILE_FIELD_COUNT;
}
