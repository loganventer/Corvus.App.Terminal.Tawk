#include "engines/attachment_policy.h"

#include <string.h>

/* Folders under the home folder that hold keys and credentials. */
static const char *const SECRET_FOLDERS[] = {
    ".ssh", ".gnupg", ".aws", ".azure", ".kube", ".docker", ".config/gcloud", ".config/gh", ".password-store", NULL
};

/* Files that hold credentials wherever they lie. */
static const char *const SECRET_NAMES[] = {
    ".netrc", ".npmrc", ".pgpass", ".env", "id_rsa", "id_dsa", "id_ecdsa", "id_ed25519", NULL
};

/* True when `path` is `base` or lies inside it. */
static int inside(const char *path, const char *base) {
    size_t n = strlen(base);
    if (n == 0) return 0;
    while (n > 1 && base[n - 1] == '/') n--;
    return strncmp(path, base, n) == 0 && (path[n] == '\0' || path[n] == '/');
}

static int in_secret_folder(const char *path, const char *home) {
    if (!home || !*home) return 0;
    size_t h = strlen(home);
    if (!inside(path, home)) return 0;
    const char *rest = path + h;
    while (*rest == '/') rest++;
    for (int i = 0; SECRET_FOLDERS[i]; i++) {
        if (inside(rest, SECRET_FOLDERS[i])) return 1;
    }
    return 0;
}

static int has_secret_name(const char *path) {
    const char *slash = strrchr(path, '/');
    const char *name = slash ? slash + 1 : path;
    for (int i = 0; SECRET_NAMES[i]; i++) {
        if (strcmp(name, SECRET_NAMES[i]) == 0) return 1;
    }
    return 0;
}

AttachmentVerdict attachment_policy_check(const char *path, const FileFacts *facts, const AttachmentRules *rules) {
    if (!path || !*path || !facts || !facts->exists) return ATTACHMENT_NOT_FOUND;
    if (!facts->is_regular) return ATTACHMENT_NOT_A_FILE;
    if (facts->size_bytes <= 0) return ATTACHMENT_EMPTY;
    if (rules && rules->max_bytes > 0 && facts->size_bytes > rules->max_bytes) return ATTACHMENT_TOO_BIG;
    for (int i = 0; rules && i < rules->protected_count; i++) {
        if (rules->protected_paths[i] && inside(path, rules->protected_paths[i])) return ATTACHMENT_PROTECTED;
    }
    if (rules && in_secret_folder(path, rules->home)) return ATTACHMENT_PROTECTED;
    if (has_secret_name(path)) return ATTACHMENT_PROTECTED;
    return ATTACHMENT_OK;
}
