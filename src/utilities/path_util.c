#include "utilities/path_util.h"
#include "utilities/app_info.h"
#include "utilities/str_util.h"

#include <errno.h>
#include <limits.h>
#include <pwd.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

/* $HOME, else the passwd entry. Never a shared directory such as /tmp,
 * where predictable paths invite symlink attacks. */
static const char *home_dir(void) {
    const char *home = getenv("HOME");
    if (home && home[0] == '/') return home;
    struct passwd *pw = getpwuid(getuid());
    return (pw && pw->pw_dir && pw->pw_dir[0] == '/') ? pw->pw_dir : "/nonexistent";
}

static void xdg_dir(char *out, size_t size, const char *env, const char *fallback) {
    const char *base = getenv(env);
    if (base && base[0] == '/') {
        snprintf(out, size, "%s/%s", base, APP_NAME);
    } else {
        snprintf(out, size, "%s/%s/%s", home_dir(), fallback, APP_NAME);
    }
}

void path_config_dir(char *out, size_t size) { xdg_dir(out, size, "XDG_CONFIG_HOME", ".config"); }
void path_data_dir(char *out, size_t size)   { xdg_dir(out, size, "XDG_DATA_HOME", ".local/share"); }
void path_cache_dir(char *out, size_t size)  { xdg_dir(out, size, "XDG_CACHE_HOME", ".cache"); }
void path_state_dir(char *out, size_t size)  { xdg_dir(out, size, "XDG_STATE_HOME", ".local/state"); }

void path_runtime_dir(char *out, size_t size) {
    const char *base = getenv("XDG_RUNTIME_DIR");
    if (base && base[0] == '/') snprintf(out, size, "%s/%s", base, APP_NAME);
    else path_state_dir(out, size);
}

void path_expand_home(const char *in, char *out, size_t size) {
    if (!in) { str_copy(out, size, ""); return; }
    if (in[0] == '~' && (in[1] == '/' || in[1] == '\0')) {
        snprintf(out, size, "%s%s", home_dir(), in + 1);
    } else {
        str_copy(out, size, in);
    }
}

int path_mkdir_p(const char *path, unsigned mode) {
    char tmp[PATH_MAX];
    if (str_copy(tmp, sizeof(tmp), path) >= sizeof(tmp)) return -1;
    for (char *p = tmp + 1; *p; p++) {
        if (*p != '/') continue;
        *p = '\0';
        if (mkdir(tmp, mode) != 0 && errno != EEXIST) return -1;
        *p = '/';
    }
    if (mkdir(tmp, mode) != 0 && errno != EEXIST) return -1;
    return 0;
}

void path_join(char *out, size_t size, const char *dir, const char *name) {
    size_t len = strlen(dir);
    snprintf(out, size, "%s%s%s", dir, (len && dir[len - 1] == '/') ? "" : "/", name);
}

int path_is_regular_file(const char *path) {
    struct stat st;
    return path && stat(path, &st) == 0 && S_ISREG(st.st_mode);
}

int path_is_within(const char *path, const char *dir) {
    char real_path[PATH_MAX], real_dir[PATH_MAX];
    if (!realpath(path, real_path) || !realpath(dir, real_dir)) return 0;
    size_t n = strlen(real_dir);
    return strncmp(real_path, real_dir, n) == 0 && real_path[n] == '/';
}

void path_download_dir(char *out, size_t size) {
    const char *env = getenv("XDG_DOWNLOAD_DIR");
    if (env && env[0] == '/') { str_copy(out, size, env); return; }
    char config[512], dirs[600];
    path_config_dir(config, sizeof(config));
    char *slash = strrchr(config, '/');                     /* ~/.config/tawk -> ~/.config */
    if (slash) *slash = '\0';
    snprintf(dirs, sizeof(dirs), "%s/user-dirs.dirs", config);
    FILE *f = fopen(dirs, "r");
    if (f) {
        char line[1024];
        while (fgets(line, sizeof(line), f)) {
            if (strncmp(line, "XDG_DOWNLOAD_DIR=\"", 18) != 0) continue;
            char *value = line + 18, *end = strchr(value, '"');
            if (!end) break;
            *end = '\0';
            if (strncmp(value, "$HOME", 5) == 0) {
                char home[512];
                path_expand_home("~", home, sizeof(home));
                snprintf(out, size, "%s%s", home, value + 5);
            } else {
                str_copy(out, size, value);
            }
            fclose(f);
            return;
        }
        fclose(f);
    }
    path_expand_home("~/Downloads", out, size);
}
