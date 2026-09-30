#include "clients/tui/media_picture.h"
#include "utilities/path_util.h"
#include "utilities/rgb_image_badge.h"
#include "utilities/rgb_image_file.h"
#include "utilities/str_util.h"

#include <ctype.h>
#include <stdio.h>
#include <string.h>
#include <strings.h>

static int ends_with_pdf(const char *s, size_t len) {
    return len >= 4 && strncasecmp(s + len - 4, ".pdf", 4) == 0;
}

int media_picture_is_pdf(const Message *m) {
    if (!m || m->type != MESSAGE_TYPE_DOCUMENT) return 0;
    if (m->media_path[0] && ends_with_pdf(m->media_path, strlen(m->media_path))) return 1;
    /* Not downloaded yet: the text starts with the file name. */
    if (!m->text) return 0;
    for (const char *p = m->text; (p = strcasestr(p, ".pdf")); p += 4) {
        if (!p[4] || isspace((unsigned char)p[4])) return 1;
    }
    return 0;
}

int media_picture_applies(const Message *m) {
    return m && !m->deleted &&
           (m->type == MESSAGE_TYPE_IMAGE || m->type == MESSAGE_TYPE_VIDEO || media_picture_is_pdf(m) ||
            (m->type == MESSAGE_TYPE_TEXT && m->link && m->thumbnail_len > 0));   /* a link card's picture */
}

int media_picture_for(const Message *m, const MediaSources *src, int page, MediaPicture *out) {
    memset(out, 0, sizeof(*out));
    out->message = m;
    out->page = page > 0 ? page : 1;
    if (!media_picture_applies(m)) return 0;
    int downloaded = m->media_path[0] && path_is_regular_file(m->media_path);
    if (m->type == MESSAGE_TYPE_IMAGE) {
        if (downloaded) { out->source = MEDIA_PICTURE_FILE; str_copy(out->path, sizeof(out->path), m->media_path); }
        else if (m->thumbnail_len > 0) out->source = MEDIA_PICTURE_THUMB;
        return out->source != MEDIA_PICTURE_NONE;
    }
    if (m->type == MESSAGE_TYPE_VIDEO) {
        if (downloaded && src && src->posters && src->posters->get(src->posters, m->media_path, out->path, sizeof(out->path))) {
            out->source = MEDIA_PICTURE_POSTER;
            return 1;
        }
    } else if (downloaded && src && src->pages &&
               src->pages->page(src->pages, m->media_path, out->page, out->path, sizeof(out->path))) {
        out->source = MEDIA_PICTURE_PAGE;
        return 1;
    }
    out->path[0] = '\0';
    /* WhatsApp's preview shows the first page only. */
    out->source = m->thumbnail_len > 0 && out->page == 1 ? MEDIA_PICTURE_THUMB : MEDIA_PICTURE_PLACEHOLDER;
    return 1;
}

int media_picture_fallback(MediaPicture *p) {
    const Message *m = p->message;
    int sharp = p->source == MEDIA_PICTURE_FILE || p->source == MEDIA_PICTURE_POSTER || p->source == MEDIA_PICTURE_PAGE;
    p->path[0] = '\0';
    if (sharp && m->thumbnail_len > 0 && p->page == 1) {
        p->source = MEDIA_PICTURE_THUMB;
        return 1;
    }
    if (p->source != MEDIA_PICTURE_PLACEHOLDER && m->type != MESSAGE_TYPE_IMAGE) {
        p->source = MEDIA_PICTURE_PLACEHOLDER;
        return 1;
    }
    p->source = MEDIA_PICTURE_NONE;
    return 0;
}

void media_picture_key(const MediaPicture *p, char *out, size_t size) {
    snprintf(out, size, "%s/%d/%d%s", p->message ? p->message->id : "", (int)p->source, p->page, p->plain ? "/plain" : "");
}

int media_picture_decode(const MediaPicture *p, RgbImage *out) {
    int rc = -1;
    int pdf = p->message->type == MESSAGE_TYPE_DOCUMENT;
    switch (p->source) {
        case MEDIA_PICTURE_FILE:
        case MEDIA_PICTURE_POSTER:
        case MEDIA_PICTURE_PAGE:        rc = rgb_image_load_file(p->path, out); break;
        case MEDIA_PICTURE_THUMB:       rc = rgb_image_decode(p->message->thumbnail, p->message->thumbnail_len, out); break;
        case MEDIA_PICTURE_PLACEHOLDER: rc = pdf ? rgb_image_page_placeholder(out) : rgb_image_video_placeholder(out); break;
        default: break;
    }
    if (rc != 0) return rc;
    if (p->message->type == MESSAGE_TYPE_VIDEO) rgb_image_draw_play_badge(out);
    else if (pdf && !p->plain) rgb_image_draw_document_badge(out);
    return 0;
}
