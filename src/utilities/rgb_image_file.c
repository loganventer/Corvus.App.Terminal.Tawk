#include "utilities/rgb_image_file.h"

#include <stdio.h>
#include <stdlib.h>

#define MAX_IMAGE_FILE (16L * 1024 * 1024)

int rgb_image_load_file(const char *path, RgbImage *out) {
    if (!path || !path[0]) return -1;
    FILE *f = fopen(path, "rb");
    if (!f) return -1;
    unsigned char *data = malloc(MAX_IMAGE_FILE);
    size_t n = data ? fread(data, 1, MAX_IMAGE_FILE, f) : 0;
    fclose(f);
    int rc = -1;
    if (n > 0 && n < (size_t)MAX_IMAGE_FILE) rc = rgb_image_decode(data, (int)n, out);
    free(data);
    return rc;
}
