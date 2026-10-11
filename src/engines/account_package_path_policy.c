#include "engines/account_package_path_policy.h"

#include <string.h>

static int climbs(const char *name) {
    for (const char *p = name; *p;) {
        const char *slash = strchr(p, '/');
        size_t len = slash ? (size_t)(slash - p) : strlen(p);
        if (len == 2 && p[0] == '.' && p[1] == '.') return 1;
        p = slash ? slash + 1 : p + len;
    }
    return 0;
}

int account_package_path_allowed(const ArchiveEntry *e) {
    const char *name = e->name;
    if (e->type != '-' && e->type != 'd') return 0;           /* links and devices */
    if (!name[0] || name[0] == '/' || climbs(name)) return 0;
    if (!strncmp(name, "./", 2)) name += 2;
    if (e->type == '-' && (!strcmp(name, "account-manifest.txt") || !strcmp(name, "package.db"))) return 1;
    return !strncmp(name, "media", 5) && (name[5] == '\0' || name[5] == '/');
}
