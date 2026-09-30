#include "engines/status_post_validator.h"
#include "engines/media_type_detector.h"
#include "engines/url_finder.h"
#include "utilities/str_util.h"

#include <stdio.h>
#include <sys/stat.h>

static int count_chars(const char *s) {
    int n = 0;
    for (; *s; s++) if (((unsigned char)*s & 0xC0) != 0x80) n++;
    return n;
}

static int blank(const char *s) {
    for (; *s; s++) if (*s != ' ' && *s != '\t' && *s != '\n' && *s != '\r') return 0;
    return 1;
}

static int fail(char *why, size_t why_size, const char *reason) {
    if (why) str_copy(why, why_size, reason);
    return -1;
}

int status_post_validate(const StatusPost *post, char *why, size_t why_size) {
    if (why && why_size) why[0] = '\0';
    if (!post) return fail(why, why_size, "There is nothing to post.");
    if (count_chars(post->text) > STATUS_POST_MAX_CHARS) {
        if (why) snprintf(why, why_size, "A status can have at most %d characters.", STATUS_POST_MAX_CHARS);
        return -1;
    }
    switch (post->kind) {
        case STATUS_KIND_TEXT:
            if (blank(post->text)) return fail(why, why_size, "Write something to post.");
            return 0;
        case STATUS_KIND_LINK: {
            char url[1024];
            if (url_find_first(post->text, url, sizeof(url)) != 0) {
                return fail(why, why_size, "Add a web address starting with http:// or https://.");
            }
            return 0;
        }
        case STATUS_KIND_PHOTO:
        case STATUS_KIND_VIDEO: {
            if (!post->path[0]) return fail(why, why_size, "Choose a file to post.");
            struct stat st;
            if (stat(post->path, &st) != 0 || !S_ISREG(st.st_mode)) return fail(why, why_size, "That file cannot be read.");
            if (st.st_size <= 0) return fail(why, why_size, "That file is empty.");
            if (st.st_size > STATUS_POST_MAX_MEDIA_BYTES) return fail(why, why_size, "Files over 100 MB cannot be posted.");
            MessageType want = post->kind == STATUS_KIND_PHOTO ? MESSAGE_TYPE_IMAGE : MESSAGE_TYPE_VIDEO;
            if (media_type_detect(post->path) != want) {
                return fail(why, why_size, post->kind == STATUS_KIND_PHOTO ? "That file is not a photo." : "That file is not a video.");
            }
            return 0;
        }
        default:
            return fail(why, why_size, "Unknown kind of status.");
    }
}
