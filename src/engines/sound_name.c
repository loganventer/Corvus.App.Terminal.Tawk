#include "engines/sound_name.h"
#include "utilities/str_util.h"

#include <string.h>
#include <strings.h>

int sound_name_is_playable(const char *file_name) {
    const char *dot = file_name ? strrchr(file_name, '.') : NULL;
    return dot && (strcasecmp(dot, ".wav") == 0 || strcasecmp(dot, ".ogg") == 0);
}

void sound_name_label(const char *path, char *out, size_t size) {
    const char *slash = path ? strrchr(path, '/') : NULL;
    str_copy(out, size, slash ? slash + 1 : (path ? path : ""));
    char *dot = strrchr(out, '.');
    if (dot && dot != out) *dot = '\0';
}
