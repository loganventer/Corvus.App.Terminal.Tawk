#ifndef APP_ENGINES_ACCOUNT_PACKAGE_PATH_POLICY_H
#define APP_ENGINES_ACCOUNT_PACKAGE_PATH_POLICY_H

#include "core/archive_entry.h"

/* Whether a member of an account package may be unpacked: plain files and
 * folders only, no absolute path, no ".." anywhere, and only account-manifest.txt,
 * package.db and what lies under media/. Anything else means the file was not
 * made by tawk or was changed. */
int account_package_path_allowed(const ArchiveEntry *entry);

#endif
