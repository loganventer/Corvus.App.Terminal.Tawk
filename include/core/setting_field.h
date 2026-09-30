#ifndef APP_CORE_SETTING_FIELD_H
#define APP_CORE_SETTING_FIELD_H

#include <stddef.h>

#include "core/setting_category.h"
#include "core/setting_kind.h"

/* Describes one Settings member: where it lives, how to persist it and how
 * the settings panel presents it. */
typedef struct SettingField {
    SettingCategory category;
    const char     *key;          /* INI key */
    const char     *label;        /* panel label */
    const char     *help;         /* panel hint and INI comment */
    SettingKind     kind;
    size_t          offset;       /* offsetof(Settings, member) */
    size_t          size;         /* buffer size for strings */
    int             min;
    int             max;
    int             step;
    const char     *choices;      /* "a|b|c" for SETTING_KIND_CHOICE */
    int             requires_restart;
} SettingField;

#endif
