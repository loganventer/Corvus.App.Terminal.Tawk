#include "core/message_file_name.h"
#include "utilities/str_util.h"

#include <ctype.h>
#include <stdio.h>
#include <string.h>

void message_file_name(const Message *m, char *out, size_t size) {
    out[0] = '\0';
    /* Documents arrive as "file name.ext caption": keep up to the extension. */
    if (m->type == MESSAGE_TYPE_DOCUMENT && m->text && m->text[0]) {
        const char *text = m->text, *end = NULL;
        for (const char *p = text; *p && *p != '\n'; p++) {
            if (*p != '.' || p == text) continue;
            const char *q = p + 1;
            while (isalnum((unsigned char)*q)) q++;
            if (q > p + 1 && q - p <= 6 && (*q == '\0' || *q == ' ' || *q == '\n')) { end = q; break; }
        }
        size_t n = end ? (size_t)(end - text) : strcspn(text, "\n");
        if (n >= size) n = size - 1;
        memcpy(out, text, n);
        out[n] = '\0';
    }
    if (!out[0] && m->media_path[0]) {
        const char *slash = strrchr(m->media_path, '/');
        str_copy(out, size, slash ? slash + 1 : m->media_path);
    }
    if (!out[0]) snprintf(out, size, "%s", m->id[0] ? m->id : "file");
    for (char *c = out; *c; c++) if (*c == '/' || *c == '\\' || (unsigned char)*c < 32) *c = '_';
    if (out[0] == '.') out[0] = '_';                              /* no hidden or ".." names */
}
