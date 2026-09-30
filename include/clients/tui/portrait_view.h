#ifndef APP_CLIENTS_TUI_PORTRAIT_VIEW_H
#define APP_CLIENTS_TUI_PORTRAIT_VIEW_H

#include "clients/tui/image_placement.h"
#include "clients/tui/thumbnail_cache.h"
#include "clients/tui/ui_rect.h"

/* Draws a contact's portrait into `rect`. With pixel images on and a
 * picture, the cells are left blank and *placement describes the image
 * (returns 1). Without pixel images a picture is drawn with half blocks
 * when the space is big enough to recognise a face; otherwise, and when
 * there is no picture, a coloured badge with the name's initials. */
int portrait_draw(UiRect rect, const char *jid, const char *name, const char *picture,
                  ThumbnailCache *thumbs, int pixel_images, ImagePlacement *placement);

#endif
