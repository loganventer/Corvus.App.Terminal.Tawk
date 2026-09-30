#ifndef APP_CORE_THEME_H
#define APP_CORE_THEME_H

#include "core/theme_color.h"
#include "core/theme_slot.h"

/* A colour theme, loaded from a JSON file (see themes/). */
typedef struct Theme {
    char       id[48];
    char       name[64];
    char       description[160];
    ThemeColor colors[THEME_SLOT_COUNT];
} Theme;

/* The built-in fallback used when no theme files can be found. */
void theme_set_default(Theme *theme);

#endif
