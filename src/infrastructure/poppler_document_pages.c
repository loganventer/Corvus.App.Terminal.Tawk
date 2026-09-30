#include "infrastructure/poppler_document_pages.h"
#include "utilities/clock_util.h"
#include "utilities/file_settled.h"
#include "utilities/process_capture.h"
#include "utilities/process_util.h"
#include "utilities/str_util.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define TRACKED   64
#define COUNTS    64
#define WAIT_MS   20000
#define PAGE_PX   "1600"      /* longest side of a rendered page */

typedef struct Render { char path[600]; int64_t started_ms; int done; } Render;
typedef struct Count { char pdf[512]; int pages; } Count;

typedef struct Pages {
    Render renders[TRACKED];
    int    next_render;
    Count  counts[COUNTS];
    int    next_count;
    int    have_pdftoppm, have_pdfinfo;
} Pages;

/* Finished: the tool has stopped writing it (a half-written file would not decode). */
static int ready(const char *path) { return file_settled(path, 300); }

static int page_count(IDocumentPages *self, const char *pdf) {
    Pages *p = self->ctx;
    if (!pdf || !pdf[0] || !p->have_pdfinfo) return 0;
    for (int i = 0; i < COUNTS; i++) if (strcmp(p->counts[i].pdf, pdf) == 0) return p->counts[i].pages;
    char out[4096];
    char *argv[] = { "pdfinfo", (char *)pdf, NULL };
    int pages = 0;
    if (process_capture(argv, out, sizeof(out), 3000) == 0) {
        const char *line = strstr(out, "\nPages:");
        if (!line && strncmp(out, "Pages:", 6) == 0) line = out - 1;
        if (line) pages = atoi(line + 7);
    }
    Count *c = &p->counts[p->next_count];
    p->next_count = (p->next_count + 1) % COUNTS;
    str_copy(c->pdf, sizeof(c->pdf), pdf);
    c->pages = pages > 0 ? pages : 0;
    return c->pages;
}

static Render *find(Pages *p, const char *path) {
    for (int i = 0; i < TRACKED; i++) if (strcmp(p->renders[i].path, path) == 0) return &p->renders[i];
    return NULL;
}

static int page(IDocumentPages *self, const char *pdf, int number, char *out, size_t size) {
    Pages *p = self->ctx;
    if (!pdf || !pdf[0] || number < 1 || strlen(pdf) + 16 >= size) return 0;
    snprintf(out, size, "%s.p%d.png", pdf, number);
    Render *r = find(p, out);
    if (ready(out)) { if (r) r->done = 1; return 1; }
    if (r || !p->have_pdftoppm) return 0;
    r = &p->renders[p->next_render];
    p->next_render = (p->next_render + 1) % TRACKED;
    str_copy(r->path, sizeof(r->path), out);
    r->started_ms = clock_now_ms();
    r->done = 0;
    char n[16], prefix[600];
    snprintf(n, sizeof(n), "%d", number);
    snprintf(prefix, sizeof(prefix), "%s.p%d", pdf, number);       /* -singlefile adds .png */
    char *const argv[] = { "pdftoppm", "-f", n, "-l", n, "-png", "-singlefile", "-scale-to", PAGE_PX,
                           (char *)pdf, prefix, NULL };
    if (process_spawn_detached(argv) != 0) r->done = 1;
    return 0;
}

static int pending(IDocumentPages *self) {
    Pages *p = self->ctx;
    int64_t now = clock_now_ms();
    int n = 0;
    for (int i = 0; i < TRACKED; i++) {
        Render *r = &p->renders[i];
        if (!r->path[0] || r->done) continue;
        if (now - r->started_ms > WAIT_MS) { r->done = 1; continue; }
        n++;                        /* until page() hands the picture to the view */
    }
    return n;
}

static void destroy(IDocumentPages *self) {
    if (!self) return;
    free(self->ctx);
    free(self);
}

IDocumentPages *poppler_document_pages_create(void) {
    IDocumentPages *d = calloc(1, sizeof(*d));
    Pages *p = calloc(1, sizeof(*p));
    if (!d || !p) { free(d); free(p); return NULL; }
    p->have_pdftoppm = process_on_path("pdftoppm");
    p->have_pdfinfo = process_on_path("pdfinfo");
    d->ctx = p;
    d->page_count = page_count;
    d->page = page;
    d->pending = pending;
    d->destroy = destroy;
    return d;
}
