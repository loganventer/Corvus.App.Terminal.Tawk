#ifndef APP_MANAGERS_SETTINGS_MANAGER_H
#define APP_MANAGERS_SETTINGS_MANAGER_H

#include "contracts/i_settings_store.h"
#include "contracts/i_theme_repository.h"
#include "core/settings.h"

/* Owns the live Settings (a stable address other components read) and
 * persists every change immediately, like the Android settings screens. */
typedef struct SettingsManager SettingsManager;

SettingsManager *settings_manager_create(ISettingsStore *store, IThemeRepository *themes);
void             settings_manager_destroy(SettingsManager *mgr);

int               settings_manager_load(SettingsManager *mgr);
/* Live settings; the pointer stays valid for the manager's lifetime. */
const Settings   *settings_manager_current(SettingsManager *mgr);
/* Replaces all settings and saves them. */
int               settings_manager_apply(SettingsManager *mgr, const Settings *updated);
IThemeRepository *settings_manager_themes(SettingsManager *mgr);
/* Goes up each time the settings change, so a screen can tell another client changed them. */
unsigned          settings_manager_revision(SettingsManager *mgr);
/* The saved theme, falling back to the default when its file is gone. */
const Theme      *settings_manager_theme(SettingsManager *mgr);

#endif
