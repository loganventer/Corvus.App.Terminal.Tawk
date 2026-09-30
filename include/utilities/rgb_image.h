#ifndef APP_UTILITIES_RGB_IMAGE_H
#define APP_UTILITIES_RGB_IMAGE_H

/* A decoded image: width * height * 3 bytes of 8-bit RGB. */
typedef struct RgbImage {
    int            width;
    int            height;
    unsigned char *pixels;
} RgbImage;

/* Decodes a JPEG or PNG held in memory. Refuses images over 4096 x 4096. */
int  rgb_image_decode(const unsigned char *data, int len, RgbImage *out);
void rgb_image_dispose(RgbImage *image);

#endif
