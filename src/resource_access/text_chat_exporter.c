#include "resource_access/text_chat_exporter.h"
#include "core/message_file_name.h"
#include "utilities/file_copy.h"
#include "utilities/path_util.h"
#include "utilities/str_util.h"

#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>

/* A chat name made safe for a file name. */
static void safe_name(const char *name, char *out, size_t size) {
    str_copy(out, size, name && name[0] ? name : "chat");
    for (char *c = out; *c; c++) if (*c == '/' || *c == '\\' || (unsigned char)*c < 32 || *c == ':') *c = '_';
    if (out[0] == '.') out[0] = '_';
}

static const char *media_word(MessageType t) {
    switch (t) {
        case MESSAGE_TYPE_IMAGE:    return "photo";
        case MESSAGE_TYPE_VIDEO:    return "video";
        case MESSAGE_TYPE_AUDIO:    return "voice note";
        case MESSAGE_TYPE_DOCUMENT: return "document";
        case MESSAGE_TYPE_STICKER:  return "sticker";
        default:                    return NULL;
    }
}

static int export_chat(IChatExporter *self, const char *chat_name, const Message *msgs, int count,
                       const char *(*sender_name)(void *, const Message *), void *names_ctx,
                       const char *dir, int with_media, char *out, size_t size) {
    (void)self;
    if (path_mkdir_p(dir, 0755) != 0) return -1;
    char base[160], stem[200], folder[900] = "", text_path[1100];
    safe_name(chat_name, base, sizeof(base));
    snprintf(stem, sizeof(stem), "tawk chat with %s", base);
    if (with_media) {
        if (file_unique_path(dir, stem, folder, sizeof(folder)) != 0 || mkdir(folder, 0755) != 0) return -1;
        snprintf(text_path, sizeof(text_path), "%s/%s.txt", folder, stem);
    } else {
        char name[240];
        snprintf(name, sizeof(name), "%s.txt", stem);
        if (file_unique_path(dir, name, text_path, sizeof(text_path)) != 0) return -1;
    }
    int fd = open(text_path, O_WRONLY | O_CREAT | O_EXCL | O_NOFOLLOW | O_CLOEXEC, 0644);
    if (fd < 0) return -1;
    FILE *f = fdopen(fd, "w");
    if (!f) { close(fd); return -1; }
    for (int i = 0; i < count; i++) {
        const Message *m = &msgs[i];
        time_t t = (time_t)m->timestamp;
        struct tm tm;
        localtime_r(&t, &tm);
        char when[32];
        strftime(when, sizeof(when), "%d/%m/%Y, %H:%M", &tm);
        const char *who = sender_name ? sender_name(names_ctx, m) : m->sender_name;
        if (m->deleted) { fprintf(f, "[%s] %s: This message was deleted\n", when, who); continue; }
        const char *word = media_word(m->type);
        if (word) {
            char file[256] = "";
            int have = m->media_path[0] && path_is_regular_file(m->media_path);
            if (with_media && have) {
                message_file_name(m, file, sizeof(file));
                char copy[1300];
                if (file_unique_path(folder, file, copy, sizeof(copy)) == 0 && file_copy(m->media_path, copy, 0644) == 0) {
                    const char *slash = strrchr(copy, '/');
                    str_copy(file, sizeof(file), slash ? slash + 1 : copy);
                } else {
                    file[0] = '\0';
                }
            }
            if (file[0]) fprintf(f, "[%s] %s: <attached: %s>\n", when, who, file);
            else fprintf(f, "[%s] %s: <%s omitted>\n", when, who, word);
            if (m->type != MESSAGE_TYPE_DOCUMENT && m->text && m->text[0]) fprintf(f, "%s\n", m->text);
            continue;
        }
        fprintf(f, "[%s] %s: %s\n", when, who, m->text ? m->text : "");
    }
    int rc = fclose(f) == 0 ? 0 : -1;
    str_copy(out, size, with_media ? folder : text_path);
    return rc;
}

static void destroy(IChatExporter *self) { free(self); }

IChatExporter *text_chat_exporter_create(void) {
    IChatExporter *e = calloc(1, sizeof(*e));
    if (!e) return NULL;
    e->export_chat = export_chat;
    e->destroy = destroy;
    return e;
}
