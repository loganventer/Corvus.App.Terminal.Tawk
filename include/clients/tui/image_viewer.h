#ifndef APP_CLIENTS_TUI_IMAGE_VIEWER_H
#define APP_CLIENTS_TUI_IMAGE_VIEWER_H

#include "clients/tui/image_placement.h"
#include "clients/tui/image_viewer_action.h"
#include "clients/tui/name_resolver.h"
#include "clients/tui/thumbnail_cache.h"
#include "clients/tui/media_sources.h"
#include "clients/tui/ui_rect.h"
#include "core/message.h"

/* A full-screen viewer inside tawk for photos, videos and PDFs: the picture
 * as large as the window allows. ← → browse the chat's media, or turn the
 * pages of a PDF (↑ ↓ then browse). Draws pixels (through the Sixel
 * overlay) or half blocks. */
typedef struct ImageViewer {
    int                 open;
    char                message_id[64];  /* the item shown; found again in the messages each frame */
    int                 page;            /* PDFs: page shown, from 1 */
    const MediaSources *sources;
    UiRect              last_rect;
    /* Portrait mode: a profile picture instead of the chat's media. */
    int                 portrait;
    char                portrait_jid[128];
    char                portrait_name[128];
    char                portrait_path[600];   /* the best picture so far; the app keeps it current */
    /* A picture file shown the same way that is not a profile picture (a
     * status photo): it keeps the file it was given and fills the window. */
    int                 file;
    char                file_label[32];       /* what it is, for the title: "status" */
} ImageViewer;

/* True for messages the viewer can show: photos, videos and PDFs. */
int               image_viewer_can_show(const Message *message);
void              image_viewer_open(ImageViewer *viewer, const Message *message, const MediaSources *sources);
void              image_viewer_close(ImageViewer *viewer);
/* Shows a contact's or group's profile picture, as large as the window allows. */
void              image_viewer_open_portrait(ImageViewer *viewer, const char *jid, const char *name, const char *path);
/* The JID whose profile picture is showing, for the app to keep it as sharp
 * as it can; NULL when it shows anything else, such as a status photo. */
const char       *image_viewer_profile_jid(const ImageViewer *viewer);
/* Shows a picture file (a status photo) as large as the window allows, titled "name · label". */
void              image_viewer_open_file(ImageViewer *viewer, const char *id, const char *name, const char *label, const char *path);
ImageViewerAction image_viewer_key(ImageViewer *viewer, const Message *messages, int count, int is_key_code, int ch);
ImageViewerAction image_viewer_wheel(ImageViewer *viewer, const Message *messages, int count, int delta);
ImageViewerAction image_viewer_click(ImageViewer *viewer, const Message *messages, int count, int y, int x);
/* The message shown, or NULL when it is no longer loaded. */
const Message    *image_viewer_current(const ImageViewer *viewer, const Message *messages, int count, int *index);
/* Draws the viewer over `area`. With pixel images on, the photo cells are
 * left blank and *placement describes where the image goes (returns 1). */
int               image_viewer_render(ImageViewer *viewer, UiRect area, const Message *messages, int count,
                                      ThumbnailCache *thumbs, const NameResolver *names, int use_24h,
                                      int pixel_images, ImagePlacement *placement);

#endif
