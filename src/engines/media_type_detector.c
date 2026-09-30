#include "engines/media_type_detector.h"

#include <string.h>
#include <strings.h>

typedef struct { const char *ext; const char *mime; MessageType type; } Kind;

static const Kind KINDS[] = {
    { ".jpg", "image/jpeg", MESSAGE_TYPE_IMAGE }, { ".jpeg", "image/jpeg", MESSAGE_TYPE_IMAGE },
    { ".png", "image/png", MESSAGE_TYPE_IMAGE },  { ".webp", "image/webp", MESSAGE_TYPE_IMAGE },
    { ".gif", "image/gif", MESSAGE_TYPE_DOCUMENT },
    { ".mp4", "video/mp4", MESSAGE_TYPE_VIDEO },  { ".mov", "video/quicktime", MESSAGE_TYPE_VIDEO },
    { ".3gp", "video/3gpp", MESSAGE_TYPE_VIDEO }, { ".m4v", "video/mp4", MESSAGE_TYPE_VIDEO },
    { ".ogg", "audio/ogg", MESSAGE_TYPE_AUDIO },  { ".opus", "audio/ogg", MESSAGE_TYPE_AUDIO },
    { ".mp3", "audio/mpeg", MESSAGE_TYPE_AUDIO }, { ".m4a", "audio/mp4", MESSAGE_TYPE_AUDIO },
    { ".aac", "audio/aac", MESSAGE_TYPE_AUDIO },  { ".wav", "audio/wav", MESSAGE_TYPE_DOCUMENT },
    { ".pdf", "application/pdf", MESSAGE_TYPE_DOCUMENT },
    { ".txt", "text/plain", MESSAGE_TYPE_DOCUMENT },
    { ".zip", "application/zip", MESSAGE_TYPE_DOCUMENT },
    { ".docx", "application/vnd.openxmlformats-officedocument.wordprocessingml.document", MESSAGE_TYPE_DOCUMENT },
    { ".xlsx", "application/vnd.openxmlformats-officedocument.spreadsheetml.sheet", MESSAGE_TYPE_DOCUMENT },
    { ".pptx", "application/vnd.openxmlformats-officedocument.presentationml.presentation", MESSAGE_TYPE_DOCUMENT },
};

#define KIND_COUNT ((int)(sizeof(KINDS) / sizeof(KINDS[0])))

const char *media_type_extension(const char *path) {
    const char *slash = strrchr(path, '/');
    const char *dot = strrchr(path, '.');
    return (dot && (!slash || dot > slash)) ? dot : "";
}

static const Kind *lookup(const char *path) {
    const char *ext = media_type_extension(path);
    for (int i = 0; i < KIND_COUNT; i++) if (strcasecmp(ext, KINDS[i].ext) == 0) return &KINDS[i];
    return NULL;
}

int media_type_known(const char *path) { return path && lookup(path) != NULL; }

MessageType media_type_detect(const char *path) {
    const Kind *k = lookup(path);
    return k ? k->type : MESSAGE_TYPE_DOCUMENT;
}

const char *media_type_mime(const char *path) {
    const Kind *k = lookup(path);
    return k ? k->mime : "application/octet-stream";
}
