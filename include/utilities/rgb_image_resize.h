#ifndef APP_UTILITIES_RGB_IMAGE_RESIZE_H
#define APP_UTILITIES_RGB_IMAGE_RESIZE_H

#include "utilities/rgb_image.h"

/* Scales an image to width x height by averaging the source pixels each
 * target pixel covers (sharp when shrinking, smooth when enlarging).
 * Returns 0 on success; free the result with rgb_image_dispose. */
int rgb_image_resize(const RgbImage *src, int width, int height, RgbImage *out);

#endif
