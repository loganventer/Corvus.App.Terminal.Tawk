#ifndef APP_CLIENTS_TUI_SIXEL_IMAGE_CACHE_H
#define APP_CLIENTS_TUI_SIXEL_IMAGE_CACHE_H

#include "clients/tui/media_picture.h"
#include "clients/tui/sixel_image.h"

/* Encodes each photo once per size and keeps the recently shown ones. */
typedef struct SixelImageCache SixelImageCache;

SixelImageCache  *sixel_image_cache_create(int capacity);
void              sixel_image_cache_destroy(SixelImageCache *cache);
/* The picture scaled to fit cols x rows cells of cell_w x cell_h pixels. */
const SixelImage *sixel_image_cache_get(SixelImageCache *cache, const MediaPicture *picture,
                                        int cols, int rows, int cell_w, int cell_h);

/* The same for a picture file (a profile picture). */
const SixelImage *sixel_image_cache_get_path(SixelImageCache *cache, const char *path, int cols, int rows, int cell_w, int cell_h,
                                             int round);

#endif
