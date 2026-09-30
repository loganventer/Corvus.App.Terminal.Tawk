#ifndef APP_RESOURCE_ACCESS_INI_SETTINGS_STORE_H
#define APP_RESOURCE_ACCESS_INI_SETTINGS_STORE_H

#include "contracts/i_settings_store.h"

/* Persists settings to an INI file at `path` (0600, parent 0700). Writes are
 * atomic (temp file plus rename). */
ISettingsStore *ini_settings_store_create(const char *path);
/* Reads only: loading never writes defaults or new keys (for --doctor). */
ISettingsStore *ini_settings_store_create_read_only(const char *path);

#endif
