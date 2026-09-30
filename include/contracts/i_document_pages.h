#ifndef APP_CONTRACTS_I_DOCUMENT_PAGES_H
#define APP_CONTRACTS_I_DOCUMENT_PAGES_H

#include <stddef.h>

/* Pages of downloaded PDF documents rendered as pictures. */
typedef struct IDocumentPages {
    void *ctx;
    /* Number of pages, or 0 when it cannot be told. */
    int  (*page_count)(struct IDocumentPages *self, const char *pdf_path);
    /* Writes the path of page `page` (from 1) to `out` and returns 1 when it
     * is rendered. Otherwise starts rendering it (once) and returns 0. */
    int  (*page)(struct IDocumentPages *self, const char *pdf_path, int page, char *out, size_t size);
    /* Non-zero while a page is being rendered (the view should redraw soon). */
    int  (*pending)(struct IDocumentPages *self);
    void (*destroy)(struct IDocumentPages *self);
} IDocumentPages;

#endif
