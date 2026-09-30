#ifndef APP_CORE_SETTING_CATEGORY_H
#define APP_CORE_SETTING_CATEGORY_H

typedef enum SettingCategory {
    SETTING_CATEGORY_APPEARANCE = 0,
    SETTING_CATEGORY_CHATS,
    SETTING_CATEGORY_NOTIFICATIONS,
    SETTING_CATEGORY_MEDIA,
    SETTING_CATEGORY_SCREENSAVER,
    SETTING_CATEGORY_RESILIENCE,
    SETTING_CATEGORY_AUTOMATION,
    SETTING_CATEGORY_ADVANCED,
    SETTING_CATEGORY_COUNT
} SettingCategory;

/* INI section name, e.g. "notifications". */
const char *setting_category_section(SettingCategory category);
/* Panel heading, e.g. "Notifications". */
const char *setting_category_title(SettingCategory category);

#endif
