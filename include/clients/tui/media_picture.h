#ifndef APP_CLIENTS_TUI_MEDIA_PICTURE_H
#define APP_CLIENTS_TUI_MEDIA_PICTURE_H

#include <stddef.h>

#include "clients/tui/media_picture_source.h"
#include "clients/tui/media_sources.h"
#include "core/message.h"
#include "utilities/rgb_image.h"

/* The best picture available for a photo, video or PDF message. Videos and
 * PDFs always have one (a placeholder at worst); videos carry a play button
 * and PDFs a document badge. */
typedef struct MediaPicture {
    const Message     *message;
    MediaPictureSource source;
    int                page;         /* PDFs: which page, from 1 */
    int                plain;        /* no document badge (the full-screen viewer) */
    char               path[600];    /* FILE, POSTER or PAGE */
} MediaPicture;

/* True for documents that are PDFs (by the downloaded file or the file name). */
int  media_picture_is_pdf(const Message *message);
/* True for messages that get a picture: photos, videos and PDFs. */
int  media_picture_applies(const Message *message);
/* Fills `out` and returns 1 when the message has a picture. `page` matters
 * for PDFs only. `sources` may be NULL. */
int  media_picture_for(const Message *message, const MediaSources *sources, int page, MediaPicture *out);
/* Steps down to the next source after a failed decode (file or poster to
 * the small preview, preview to the placeholder for videos). Returns 0
 * when there is nothing left. */
int  media_picture_fallback(MediaPicture *picture);
/* Cache key: message id and source, so a sharper source replaces a blurry one. */
void media_picture_key(const MediaPicture *picture, char *out, size_t size);
/* Decodes the picture, with the play button drawn on videos and the badge on PDFs. */
int  media_picture_decode(const MediaPicture *picture, RgbImage *out);

#endif
