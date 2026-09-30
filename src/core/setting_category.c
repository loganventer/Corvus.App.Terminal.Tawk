#include "core/setting_category.h"

static const char *const SECTIONS[SETTING_CATEGORY_COUNT] = {
    "appearance", "chats", "notifications", "media", "screensaver", "resilience", "automation", "advanced"
};
static const char *const TITLES[SETTING_CATEGORY_COUNT] = {
    "Appearance", "Chats", "Notifications", "Media", "Screensaver", "Resilience", "Automation", "Advanced"
};

const char *setting_category_section(SettingCategory c) {
    return (c >= 0 && c < SETTING_CATEGORY_COUNT) ? SECTIONS[c] : "general";
}

const char *setting_category_title(SettingCategory c) {
    return (c >= 0 && c < SETTING_CATEGORY_COUNT) ? TITLES[c] : "General";
}
