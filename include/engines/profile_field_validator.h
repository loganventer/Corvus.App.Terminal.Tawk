#ifndef APP_ENGINES_PROFILE_FIELD_VALIDATOR_H
#define APP_ENGINES_PROFILE_FIELD_VALIDATOR_H

#include <stddef.h>

#include "core/profile_field.h"

/* WhatsApp's limits, in characters: a name of 1 to 25, an about of up to 139. */
int profile_field_max_chars(ProfileField field);
/* 0 when `text` (UTF-8) may be saved as `field`; otherwise -1 and a reason for people in `why`. */
int profile_field_validate(ProfileField field, const char *text, char *why, size_t why_size);

#endif
