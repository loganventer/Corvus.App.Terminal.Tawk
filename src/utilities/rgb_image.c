#include "utilities/rgb_image.h"

#include <stdlib.h>
#include <string.h>

/* stb declares loaders for formats we compile out; the unused-function
 * check runs at the end of the file, after any pragma scope. */
#if defined(__GNUC__)
#pragma GCC diagnostic ignored "-Wunused-function"
#endif

/* Only the decoders tawk needs; no file I/O, no HDR, no GIF animation. */
#define STBI_ONLY_JPEG
#define STBI_ONLY_PNG
#define STBI_NO_STDIO
#define STBI_NO_HDR
#define STBI_NO_LINEAR
#define STBI_MAX_DIMENSIONS 4096
#define STB_IMAGE_STATIC
#define STB_IMAGE_IMPLEMENTATION
#if defined(__GNUC__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wshadow"
#pragma GCC diagnostic ignored "-Wsign-compare"
#pragma GCC diagnostic ignored "-Wunused-function"
#pragma GCC diagnostic ignored "-Wimplicit-fallthrough"
#pragma GCC diagnostic ignored "-Wmisleading-indentation"
#pragma GCC diagnostic ignored "-Wpedantic"
#pragma GCC diagnostic ignored "-Wformat-nonliteral"
#endif
#include "stb_image.h"
#if defined(__GNUC__)
#pragma GCC diagnostic pop
#endif

int rgb_image_decode(const unsigned char *data, int len, RgbImage *out) {
    memset(out, 0, sizeof(*out));
    if (!data || len <= 0) return -1;
    int w = 0, h = 0, channels = 0;
    unsigned char *pixels = stbi_load_from_memory(data, len, &w, &h, &channels, 3);
    if (!pixels || w <= 0 || h <= 0) {
        if (pixels) stbi_image_free(pixels);
        return -1;
    }
    out->width = w;
    out->height = h;
    out->pixels = pixels;
    return 0;
}

void rgb_image_dispose(RgbImage *image) {
    if (image && image->pixels) stbi_image_free(image->pixels);
    if (image) memset(image, 0, sizeof(*image));
}
