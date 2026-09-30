#ifndef APP_CORE_PROFILE_EDIT_RESULT_H
#define APP_CORE_PROFILE_EDIT_RESULT_H

#include "core/profile_field.h"

/* How a change to your own profile ended. */
typedef struct ProfileEditResult {
    ProfileField field;
    int          ok;
    char         detail[256];   /* why it failed; empty on success */
} ProfileEditResult;

#endif
