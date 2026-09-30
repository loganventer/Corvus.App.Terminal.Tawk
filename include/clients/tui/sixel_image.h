#ifndef APP_CLIENTS_TUI_SIXEL_IMAGE_H
#define APP_CLIENTS_TUI_SIXEL_IMAGE_H

#include <stddef.h>

/* A picture encoded as a Sixel escape sequence, ready to write. */
typedef struct SixelImage {
    char  *data;
    size_t length;
} SixelImage;

#endif
