#ifndef APP_UTILITIES_RGB_IMAGE_FILE_H
#define APP_UTILITIES_RGB_IMAGE_FILE_H

#include "utilities/rgb_image.h"

/* Decodes a JPEG or PNG file of at most 16 MB. Returns 0 on success. */
int rgb_image_load_file(const char *path, RgbImage *out);

#endif
