#ifndef APP_INFRASTRUCTURE_POPPLER_DOCUMENT_PAGES_H
#define APP_INFRASTRUCTURE_POPPLER_DOCUMENT_PAGES_H

#include "contracts/i_document_pages.h"

/* Renders PDF pages with pdftoppm and counts them with pdfinfo (poppler),
 * in the background; pages are saved beside the PDF as <pdf>.p<N>.png.
 * Reports nothing when poppler is not installed. */
IDocumentPages *poppler_document_pages_create(void);

#endif
