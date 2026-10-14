#ifndef APP_RESOURCE_ACCESS_FOLDER_SOUND_CATALOG_H
#define APP_RESOURCE_ACCESS_FOLDER_SOUND_CATALOG_H

#include "contracts/i_sound_catalog.h"

/* Lists the .wav and .ogg files of the bundled folder, then of the user
 * folder (a user file with the same name replaces the bundled one). Either
 * folder may be missing. Sorted by name. */
ISoundCatalog *folder_sound_catalog_create(const char *bundled_dir, const char *user_dir);

#endif
