#ifndef APP_INFRASTRUCTURE_SYSTEM_CLIPBOARD_IMAGE_H
#define APP_INFRASTRUCTURE_SYSTEM_CLIPBOARD_IMAGE_H

#include "contracts/i_clipboard_image.h"

/* Uses whatever the system has: wl-paste (Wayland, including WSLg), xclip
 * (X11), pngpaste (macOS), or PowerShell when Windows interop is on. */
IClipboardImage *system_clipboard_image_create(void);

#endif
