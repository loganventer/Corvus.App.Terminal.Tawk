#include "core/theme_slot.h"

#include <string.h>

static const char *const KEYS[THEME_SLOT_COUNT] = {
    "base", "header", "sidebar", "sidebar_selected", "badge", "chat", "bubble_me",
    "bubble_them", "sender", "timestamp", "day_separator", "composer", "accent", "ok",
    "warn", "blink", "media", "dim", "border", "unread"
};

const char *theme_slot_key(ThemeSlot slot) {
    return (slot >= 0 && slot < THEME_SLOT_COUNT) ? KEYS[slot] : "";
}

int theme_slot_parse(const char *key) {
    for (int i = 0; key && i < THEME_SLOT_COUNT; i++) {
        if (strcmp(KEYS[i], key) == 0) return i;
    }
    return -1;
}
