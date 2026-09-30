#ifndef APP_CORE_THEME_SLOT_H
#define APP_CORE_THEME_SLOT_H

/* Every coloured surface in the UI. Each slot maps to one curses colour pair. */
typedef enum ThemeSlot {
    THEME_SLOT_BASE = 0,
    THEME_SLOT_HEADER,
    THEME_SLOT_SIDEBAR,
    THEME_SLOT_SIDEBAR_SELECTED,
    THEME_SLOT_BADGE,
    THEME_SLOT_CHAT,
    THEME_SLOT_BUBBLE_ME,
    THEME_SLOT_BUBBLE_THEM,
    THEME_SLOT_SENDER,
    THEME_SLOT_TIMESTAMP,
    THEME_SLOT_DAY_SEPARATOR,
    THEME_SLOT_COMPOSER,
    THEME_SLOT_ACCENT,
    THEME_SLOT_OK,
    THEME_SLOT_WARN,
    THEME_SLOT_BLINK,
    THEME_SLOT_MEDIA,
    THEME_SLOT_DIM,
    THEME_SLOT_BORDER,
    THEME_SLOT_UNREAD,      /* chat rows with unread messages */
    THEME_SLOT_COUNT
} ThemeSlot;

/* JSON key for a slot, e.g. "bubble_me". */
const char *theme_slot_key(ThemeSlot slot);
/* Returns the slot for a key, or -1. */
int         theme_slot_parse(const char *key);

#endif
