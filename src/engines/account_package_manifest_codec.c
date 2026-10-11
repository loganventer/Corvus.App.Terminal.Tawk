#include "engines/account_package_manifest_codec.h"
#include "utilities/str_util.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int account_package_manifest_format(const AccountPackageManifest *m, char *out, size_t size) {
    int n = snprintf(out, size,
                     "# tawk account package\n"
                     "format=%d\nversion=%s\nschema=%d\ncreated=%lld\naccount=%d\n"
                     "label=%s\njid=%s\nname=%s\ncolour=%d\nmedia_dir=%s\nmedia=%d\n",
                     m->format, m->app_version, m->schema_version, (long long)m->created, m->account_id,
                     m->label, m->jid, m->name, m->colour, m->media_dir, m->has_media);
    return n > 0 && (size_t)n < size ? 0 : -1;
}

static void set_field(AccountPackageManifest *m, const char *key, const char *value) {
    int v = atoi(value);
    if (!strcmp(key, "format")) m->format = v;
    else if (!strcmp(key, "version")) str_copy(m->app_version, sizeof(m->app_version), value);
    else if (!strcmp(key, "schema")) m->schema_version = v;
    else if (!strcmp(key, "created")) m->created = strtoll(value, NULL, 10);
    else if (!strcmp(key, "account")) m->account_id = v;
    else if (!strcmp(key, "label")) str_copy(m->label, sizeof(m->label), value);
    else if (!strcmp(key, "jid")) str_copy(m->jid, sizeof(m->jid), value);
    else if (!strcmp(key, "name")) str_copy(m->name, sizeof(m->name), value);
    else if (!strcmp(key, "colour")) m->colour = v;
    else if (!strcmp(key, "media_dir")) str_copy(m->media_dir, sizeof(m->media_dir), value);
    else if (!strcmp(key, "media")) m->has_media = v != 0;
}

int account_package_manifest_parse(const char *text, AccountPackageManifest *out) {
    memset(out, 0, sizeof(*out));
    for (const char *line = text; line && *line;) {
        const char *end = strchr(line, '\n');
        size_t len = end ? (size_t)(end - line) : strlen(line);
        char buf[700];
        if (len < sizeof(buf) && line[0] != '#') {
            memcpy(buf, line, len);
            buf[len] = '\0';
            char *eq = strchr(buf, '=');
            if (eq) { *eq = '\0'; set_field(out, buf, eq + 1); }
        }
        line = end ? end + 1 : NULL;
    }
    return out->format >= 1 && out->format <= ACCOUNT_PACKAGE_FORMAT ? 0 : -1;
}
