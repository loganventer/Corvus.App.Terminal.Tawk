#ifndef APP_CLIENTS_TUI_SIXEL_OVERLAY_H
#define APP_CLIENTS_TUI_SIXEL_OVERLAY_H

#include "clients/tui/image_placement.h"
#include "clients/tui/sixel_image_cache.h"
#include "core/message.h"

#define SIXEL_OVERLAY_MAX 48

/* Draws photos as Sixel images over the blank cells the message view left
 * for them. curses does not know about the pixels, so images are written
 * right after the screen update, and only when they moved, appeared, or
 * curses repainted the cells under them. */
typedef struct SixelOverlay {
    ImagePlacement shown[SIXEL_OVERLAY_MAX];
    int            shown_count;
    int            lines, cols;
    int            stale;
} SixelOverlay;

/* Forces the next present to redraw every image (after a full repaint). */
void sixel_overlay_invalidate(SixelOverlay *overlay);
/* Updates the screen (refresh), then writes the images that need it. */
void sixel_overlay_present(SixelOverlay *overlay, SixelImageCache *cache, int cell_w, int cell_h,
                           const ImagePlacement *placements, int count, const Message *messages, int message_count);

#endif
