#include "resource_access/file_admin_token_store.h"
#include "utilities/str_util.h"

#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

typedef struct FileAdminTokenStore {
    char path[600];
} FileAdminTokenStore;

/* Written beside itself and renamed, so a reader never sees half a token. */
static int token_save(IAdminTokenStore *self, const char *token) {
    FileAdminTokenStore *s = self->ctx;
    char part[640];
    snprintf(part, sizeof(part), "%s.part", s->path);
    unlink(part);
    int fd = open(part, O_WRONLY | O_CREAT | O_EXCL | O_CLOEXEC | O_NOFOLLOW, 0600);
    if (fd < 0) return -1;
    size_t n = strlen(token);
    int ok = write(fd, token, n) == (ssize_t)n && write(fd, "\n", 1) == 1;
    if (close(fd) != 0) ok = 0;
    if (!ok || rename(part, s->path) != 0) { unlink(part); return -1; }
    return 0;
}

static void token_remove(IAdminTokenStore *self) {
    FileAdminTokenStore *s = self->ctx;
    unlink(s->path);
}

static void token_destroy(IAdminTokenStore *self) {
    free(self->ctx);
    free(self);
}

IAdminTokenStore *file_admin_token_store_create(const char *path) {
    IAdminTokenStore *self = calloc(1, sizeof(*self));
    FileAdminTokenStore *s = calloc(1, sizeof(*s));
    if (!self || !s) { free(self); free(s); return NULL; }
    str_copy(s->path, sizeof(s->path), path);
    self->ctx = s;
    self->save = token_save;
    self->remove = token_remove;
    self->destroy = token_destroy;
    return self;
}
