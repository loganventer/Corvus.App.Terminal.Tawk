#include "infrastructure/stored_passphrase_prompt.h"
#include "utilities/app_info.h"
#include "utilities/secure_zero.h"

#include <fcntl.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

typedef struct StoredPassphrase {
    IPassphrasePrompt *fallback;
    char              *path;
    int                answered;               /* the key file has been tried once */
} StoredPassphrase;

/* Reads the key file into `out`. Returns 0 when it held a usable passphrase. */
static int read_key_file(const char *path, Passphrase *out) {
    int fd = open(path, O_RDONLY | O_CLOEXEC | O_NOFOLLOW);
    if (fd < 0) return -1;
    struct stat st;
    if (fstat(fd, &st) != 0 || !S_ISREG(st.st_mode) || st.st_uid != getuid()) { close(fd); return -1; }
    if ((st.st_mode & 077) != 0) {
        fprintf(stderr, "%s: %s is readable by others, so it is not used. Run: chmod 600 %s\n", APP_NAME, path, path);
        close(fd);
        return -1;
    }
    char buf[PASSPHRASE_MAX + 4];
    ssize_t n = read(fd, buf, sizeof(buf) - 1);
    close(fd);
    if (n <= 0) { secure_zero(buf, sizeof(buf)); return -1; }
    buf[n] = '\0';
    while (n > 0 && (buf[n - 1] == '\n' || buf[n - 1] == '\r')) buf[--n] = '\0';
    int rc = n > 0 ? passphrase_set(out, buf) : -1;
    secure_zero(buf, sizeof(buf));
    return rc;
}

static int ask(IPassphrasePrompt *self, const char *prompt, int confirm, Passphrase *out) {
    StoredPassphrase *s = self->ctx;
    if (!confirm && !s->answered && s->path) {
        s->answered = 1;
        if (read_key_file(s->path, out) == 0) return 0;
    }
    return s->fallback->ask(s->fallback, prompt, confirm, out);
}

static void destroy(IPassphrasePrompt *self) {
    StoredPassphrase *s = self->ctx;
    free(s->path);
    free(s);
    free(self);
}

IPassphrasePrompt *stored_passphrase_prompt_create(IPassphrasePrompt *fallback, const char *path) {
    if (!fallback) return NULL;
    IPassphrasePrompt *p = calloc(1, sizeof(*p));
    StoredPassphrase *s = calloc(1, sizeof(*s));
    if (!p || !s) { free(p); free(s); return NULL; }
    s->fallback = fallback;
    if (path && *path) {
        s->path = strdup(path);
        if (!s->path) { free(p); free(s); return NULL; }
    }
    p->ctx = s;
    p->ask = ask;
    p->destroy = destroy;
    return p;
}

IPassphrasePrompt *stored_passphrase_prompt_create_default(IPassphrasePrompt *fallback, const char *config_path) {
    char path[PATH_MAX];
    if (!config_path || !*config_path) return stored_passphrase_prompt_create(fallback, NULL);
    const char *slash = strrchr(config_path, '/');
    int n = slash ? snprintf(path, sizeof(path), "%.*s/db.key", (int)(slash - config_path), config_path)
                  : snprintf(path, sizeof(path), "db.key");
    if (n < 0 || (size_t)n >= sizeof(path)) return stored_passphrase_prompt_create(fallback, NULL);
    return stored_passphrase_prompt_create(fallback, path);
}
