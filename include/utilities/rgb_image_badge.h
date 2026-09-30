#ifndef APP_UTILITIES_RGB_IMAGE_BADGE_H
#define APP_UTILITIES_RGB_IMAGE_BADGE_H

#include "utilities/rgb_image.h"

/* Draws a play button (a dark translucent disc with a white triangle) in
 * the middle of the image, marking it as a video. */
void rgb_image_draw_play_badge(RgbImage *image);
/* A plain dark 16:9 picture for a video that has no preview. */
int  rgb_image_video_placeholder(RgbImage *out);
/* A red "document" tab in the bottom-left corner, marking a PDF. */
void rgb_image_draw_document_badge(RgbImage *image);
/* A blank A4 page with grey text lines, for a PDF that has no preview yet. */
int  rgb_image_page_placeholder(RgbImage *out);

#endif
