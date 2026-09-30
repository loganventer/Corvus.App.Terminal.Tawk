#include "core/link_preview.h"
#include "utilities/str_util.h"

#include <stdlib.h>

LinkPreview *link_preview_create(const char *url, const char *title, const char *description) {
    if (!url || !*url) return NULL;
    LinkPreview *p = calloc(1, sizeof(*p));
    if (!p) return NULL;
    str_copy(p->url, sizeof(p->url), url);
    str_copy(p->title, sizeof(p->title), title ? title : "");
    str_copy(p->description, sizeof(p->description), description ? description : "");
    return p;
}

LinkPreview *link_preview_copy(const LinkPreview *src) {
    return src ? link_preview_create(src->url, src->title, src->description) : NULL;
}
