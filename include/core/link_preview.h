#ifndef APP_CORE_LINK_PREVIEW_H
#define APP_CORE_LINK_PREVIEW_H

/* The card a message shows for the web address in it; the picture is the
 * message's thumbnail. */
typedef struct LinkPreview {
    char url[512];
    char title[256];
    char description[512];
} LinkPreview;

/* A heap copy of the given fields, or NULL when url is empty. */
LinkPreview *link_preview_create(const char *url, const char *title, const char *description);
LinkPreview *link_preview_copy(const LinkPreview *src);

#endif
