#ifndef APP_INFRASTRUCTURE_TAR_ARCHIVE_H
#define APP_INFRASTRUCTURE_TAR_ARCHIVE_H

#include "contracts/i_archive.h"

/* Gzipped tar archives made and read with the system's tar (GNU tar on
 * Linux, bsdtar on macOS), run with argument lists and never a shell. */
IArchive *tar_archive_create(void);

#endif
