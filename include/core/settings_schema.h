#ifndef APP_CORE_SETTINGS_SCHEMA_H
#define APP_CORE_SETTINGS_SCHEMA_H

#include "core/setting_field.h"
#include "core/settings.h"

int                 settings_schema_count(void);
const SettingField *settings_schema_at(int index);
const SettingField *settings_schema_find(SettingCategory category, const char *key);

int         setting_get_int(const Settings *settings, const SettingField *field);
void        setting_set_int(Settings *settings, const SettingField *field, int value);
const char *setting_get_string(const Settings *settings, const SettingField *field);
/* Control characters are stripped so values stay single-line. */
void        setting_set_string(Settings *settings, const SettingField *field, const char *value);
/* Parses and stores a textual value; bounds are enforced. */
void        setting_set_from_text(Settings *settings, const SettingField *field, const char *text);
/* Renders the value as text, e.g. for the INI file. */
void        setting_to_text(const Settings *settings, const SettingField *field, char *out, size_t size);

#endif
