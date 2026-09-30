#ifndef APP_INFRASTRUCTURE_SYSTEM_MEDIA_OPENER_H
#define APP_INFRASTRUCTURE_SYSTEM_MEDIA_OPENER_H

#include "contracts/i_media_opener.h"
#include "core/settings.h"

/* Opens files in the user's default application: the configured viewer when
 * set, otherwise open (macOS), wslview/explorer.exe (WSL with interop),
 * cygstart (MSYS2/Cygwin) or xdg-open/gio (Linux). Borrows settings. */
IMediaOpener *system_media_opener_create(const Settings *settings);

#endif
