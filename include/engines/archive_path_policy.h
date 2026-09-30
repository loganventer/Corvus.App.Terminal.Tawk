#ifndef APP_ENGINES_ARCHIVE_PATH_POLICY_H
#define APP_ENGINES_ARCHIVE_PATH_POLICY_H

#include "core/archive_entry.h"

/* Whether a backup member may be extracted: only plain files and folders,
 * no absolute paths, no ".." anywhere, and only under the entries a backup
 * holds (manifest.txt, tawk.db and its log, config.ini, themes/, media/,
 * auth/). Anything else means the file was not made by tawk or was changed. */
int archive_path_allowed(const ArchiveEntry *entry);

#endif
