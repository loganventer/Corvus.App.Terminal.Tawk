#ifndef APP_CONTRACTS_I_CLIPBOARD_IMAGE_H
#define APP_CONTRACTS_I_CLIPBOARD_IMAGE_H

#include <stddef.h>

typedef enum ClipboardImageResult {
    CLIPBOARD_IMAGE_SAVED = 0,
    CLIPBOARD_IMAGE_EMPTY,        /* the clipboard holds no picture */
    CLIPBOARD_IMAGE_NO_TOOL       /* no way to read the clipboard on this system */
} ClipboardImageResult;

/* Reads a picture from the system clipboard (which a terminal paste cannot
 * carry) and saves it as a file. */
typedef struct IClipboardImage {
    void *ctx;
    /* Saves the picture into `dir`; the file's path goes to `out`. */
    ClipboardImageResult (*save)(struct IClipboardImage *self, const char *dir, char *out, size_t size);
    void (*destroy)(struct IClipboardImage *self);
} IClipboardImage;

#endif
