#include "core/theme.h"
#include "utilities/str_util.h"

#include <string.h>

void theme_set_default(Theme *t) {
    static const ThemeColor COLORS[THEME_SLOT_COUNT] = {
        {252,234},{255,236},{252,234},{255,238},{16,36},{252,233},{255,23},{255,236},{79,236},{245,234},
        {250,237},{255,236},{36,234},{36,236},{214,236},{16,36},{81,233},{243,234},{239,234},
        {48,236}
    };
    memset(t, 0, sizeof(*t));
    str_copy(t->id, sizeof(t->id), "whatsapp-dark");
    str_copy(t->name, sizeof(t->name), "WhatsApp Dark");
    str_copy(t->description, sizeof(t->description), "Built-in default");
    memcpy(t->colors, COLORS, sizeof(COLORS));
}
