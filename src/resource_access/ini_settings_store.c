#include "resource_access/ini_settings_store.h"
#include "core/settings_schema.h"
#include "utilities/app_info.h"
#include "utilities/log.h"
#include "utilities/path_util.h"
#include "utilities/str_util.h"

#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

typedef struct IniStore {
    char path[PATH_MAX];
    int  read_only;
} IniStore;

static IniStore *ctx_of(ISettingsStore *self) { return (IniStore *)self->ctx; }

static int category_from_section(const char *section) {
    for (int c = 0; c < SETTING_CATEGORY_COUNT; c++) {
        if (strcmp(section, setting_category_section((SettingCategory)c)) == 0) return c;
    }
    return -1;
}

/* A config that anyone else can modify must not be allowed to choose
 * commands we execute. */
static int file_is_trusted(const char *path) {
    struct stat st;
    if (stat(path, &st) != 0) return 1;
    if (st.st_uid != getuid()) return 0;
    return (st.st_mode & (S_IWGRP | S_IWOTH)) == 0;
}

/* Keys whose values end up in exec(); ignored when the file is untrusted. */
static int field_executes(const SettingField *field) {
    static const char *const KEYS[] = {
        "command", "image_viewer", "video_player", "node_binary", "sidecar_dir", NULL
    };
    for (int i = 0; KEYS[i]; i++) if (strcmp(field->key, KEYS[i]) == 0) return 1;
    return 0;
}

static int store_save(ISettingsStore *self, const Settings *s) {
    const char *path = ctx_of(self)->path;
    char dir[PATH_MAX];
    str_copy(dir, sizeof(dir), path);
    char *slash = strrchr(dir, '/');
    if (slash) { *slash = '\0'; path_mkdir_p(dir, 0700); }

    char tmp[PATH_MAX + 8];
    snprintf(tmp, sizeof(tmp), "%s.tmp", path);
    int fd = open(tmp, O_WRONLY | O_CREAT | O_TRUNC | O_CLOEXEC | O_NOFOLLOW, 0600);
    if (fd < 0) { LOG_ERROR("cannot write %s: %s", tmp, strerror(errno)); return -1; }
    FILE *f = fdopen(fd, "w");
    if (!f) { close(fd); return -1; }

    fprintf(f, "# %s configuration (written by %s %s)\n", APP_NAME, APP_NAME, APP_VERSION);
    fprintf(f, "# Edit here or in the settings panel (F2 / click the gear). Changes made in\n");
    fprintf(f, "# the panel are saved straight back to this file.\n");
    for (int c = 0; c < SETTING_CATEGORY_COUNT; c++) {
        fprintf(f, "\n[%s]\n", setting_category_section((SettingCategory)c));
        for (int i = 0; i < settings_schema_count(); i++) {
            const SettingField *field = settings_schema_at(i);
            if ((int)field->category != c) continue;
            char value[1024];
            setting_to_text(s, field, value, sizeof(value));
            fprintf(f, "# %s%s\n", field->help, field->requires_restart ? " (restart required)" : "");
            fprintf(f, "%s = %s\n", field->key, value);
        }
    }
    int ok = fflush(f) == 0 && fsync(fd) == 0;
    fclose(f);
    if (!ok || rename(tmp, path) != 0) {
        LOG_ERROR("cannot save settings to %s", path);
        unlink(tmp);
        return -1;
    }
    return 0;
}

static int store_load(ISettingsStore *self, Settings *s) {
    const char *path = ctx_of(self)->path;
    str_copy(s->config_path, sizeof(s->config_path), path);
    FILE *f = fopen(path, "r");
    if (!f) {
        s->config_trusted = 1;
        if (ctx_of(self)->read_only) return 0;
        LOG_INFO("no config at %s, writing defaults", path);
        return store_save(self, s);
    }
    s->config_trusted = file_is_trusted(path);
    if (!s->config_trusted) {
        LOG_WARN("%s is writable by other users; command settings are ignored until fixed (chmod 600)", path);
    }

    char line[2048];
    int category = -1;
    while (fgets(line, sizeof(line), f)) {
        char *text = str_trim(line);
        if (!*text || *text == '#' || *text == ';') continue;
        if (*text == '[') {
            char *end = strchr(text, ']');
            if (end) *end = '\0';
            category = category_from_section(str_trim(text + 1));
            continue;
        }
        char *eq = strchr(text, '=');
        if (!eq || category < 0) continue;
        *eq = '\0';
        const SettingField *field = settings_schema_find((SettingCategory)category, str_trim(text));
        if (!field) continue;
        if (!s->config_trusted && field_executes(field)) continue;
        setting_set_from_text(s, field, str_trim(eq + 1));
    }
    fclose(f);
    /* Re-save so new keys from later versions appear, but never rewrite (and
     * thereby launder) a file someone else could have tampered with. */
    return s->config_trusted && !ctx_of(self)->read_only ? store_save(self, s) : 0;
}

static void store_destroy(ISettingsStore *self) {
    free(self->ctx);
    free(self);
}

ISettingsStore *ini_settings_store_create(const char *path) {
    ISettingsStore *s = calloc(1, sizeof(*s));
    IniStore *ctx = calloc(1, sizeof(*ctx));
    if (!s || !ctx) { free(s); free(ctx); return NULL; }
    str_copy(ctx->path, sizeof(ctx->path), path);
    s->ctx = ctx;
    s->load = store_load;
    s->save = store_save;
    s->destroy = store_destroy;
    return s;
}

ISettingsStore *ini_settings_store_create_read_only(const char *path) {
    ISettingsStore *s = ini_settings_store_create(path);
    if (s) ((IniStore *)s->ctx)->read_only = 1;
    return s;
}
