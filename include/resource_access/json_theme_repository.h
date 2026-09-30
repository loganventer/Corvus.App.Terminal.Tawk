#ifndef APP_RESOURCE_ACCESS_JSON_THEME_REPOSITORY_H
#define APP_RESOURCE_ACCESS_JSON_THEME_REPOSITORY_H

#include "contracts/i_theme_repository.h"

/* Loads *.json themes from the bundled folder, then the user folder (a user
 * theme with the same id replaces the bundled one). Always contains at least
 * the built-in default. Themes are sorted by name. */
IThemeRepository *json_theme_repository_create(const char *bundled_dir, const char *user_dir);

#endif
