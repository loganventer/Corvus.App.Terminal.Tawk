#ifndef APP_CLIENTS_TUI_THUMBNAIL_CACHE_H
#define APP_CLIENTS_TUI_THUMBNAIL_CACHE_H

#include "clients/tui/media_picture.h"
#include "clients/tui/thumbnail.h"
#include "utilities/rgb_image.h"

/* Decodes pictures once and keeps the cell version of recently shown
 * ones, keyed by message id and width. */
typedef struct ThumbnailCache ThumbnailCache;

ThumbnailCache  *thumbnail_cache_create(int capacity);
void             thumbnail_cache_destroy(ThumbnailCache *cache);
/* Cells for the picture at most max_cols wide and max_rows tall, or NULL.
 * Falls back to a blurrier source when one cannot be decoded, updating
 * *picture to the source used. */
const Thumbnail *thumbnail_cache_get(ThumbnailCache *cache, MediaPicture *picture, int max_cols, int max_rows);
/* Cells for a picture file (a profile picture) at most max_cols x max_rows, or NULL. */
const Thumbnail *thumbnail_cache_get_path(ThumbnailCache *cache, const char *path, int max_cols, int max_rows);
/* Cells for an image already in memory (a camera frame), not cached; free with thumbnail_free. */
Thumbnail       *thumbnail_from_image(const RgbImage *image, int max_cols, int max_rows);
void             thumbnail_free(Thumbnail *thumb);
/* Draws a thumbnail with its top-left corner at (y, x). */
void             thumbnail_draw(const Thumbnail *thumb, int y, int x);

#endif
