#ifndef APP_CLIENTS_TUI_MEDIA_SOURCES_H
#define APP_CLIENTS_TUI_MEDIA_SOURCES_H

#include "contracts/i_document_pages.h"
#include "contracts/i_video_poster.h"

/* Where pictures of media that are not photos come from. Either may be NULL. */
typedef struct MediaSources {
    IVideoPoster   *posters;     /* frames of downloaded videos */
    IDocumentPages *pages;       /* pages of downloaded PDFs */
} MediaSources;

#endif
