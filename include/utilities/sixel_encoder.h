#ifndef APP_UTILITIES_SIXEL_ENCODER_H
#define APP_UTILITIES_SIXEL_ENCODER_H

#include <stddef.h>

#include "utilities/rgb_image.h"

/* Encodes an image as a DEC Sixel escape sequence (DCS ... ST) with a
 * palette of up to 256 colours chosen from the image. Returns a
 * NUL-terminated string the caller frees, or NULL. */
char *sixel_encode(const RgbImage *image, size_t *length);
/* The same, drawing only pixels whose mask byte is non-zero (width x
 * height bytes); the rest stay transparent. NULL mask draws everything. */
char *sixel_encode_masked(const RgbImage *image, const unsigned char *mask, size_t *length);

#endif
