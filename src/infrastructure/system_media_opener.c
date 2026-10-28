#include "infrastructure/system_media_opener.h"
#include "infrastructure/app_launcher.h"
#include "utilities/log.h"
#include "utilities/path_util.h"
#include "utilities/platform.h"
#include "utilities/process_util.h"
#include "utilities/str_util.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct Opener {
    const Settings *settings;
} Opener;

/* A desktop's default for video is sometimes an image viewer; a real player
 * found on PATH is the safer first choice. ffplay comes with ffmpeg. */
static int open_video(const char *path) {
    static const char *const PLAYERS[] = { "mpv", "vlc", "celluloid", "totem" };
    for (size_t i = 0; i < sizeof(PLAYERS) / sizeof(PLAYERS[0]); i++) {
        if (app_launcher_try_tool(PLAYERS[i], path) == 0) return 0;
    }
    if (process_on_path("ffplay")) {
        char *const argv[] = { "ffplay", "-autoexit", "-loglevel", "error", (char *)path, NULL };
        return app_launcher_spawn(argv);
    }
    return -1;
}

/* "builtin" and "system" are not commands. */
static int is_command(const char *value) {
    return value[0] && strcmp(value, "builtin") != 0 && strcmp(value, "system") != 0;
}

static int opener_open(IMediaOpener *self, const char *path, MessageType type) {
    Opener *o = self->ctx;
    if (!path_is_regular_file(path)) return -1;
    const Settings *s = o->settings;
    const char *custom = type == MESSAGE_TYPE_VIDEO ? s->video_player :
                         (type == MESSAGE_TYPE_IMAGE || type == MESSAGE_TYPE_STICKER) ? s->image_viewer : "";
    if (is_command(custom) && s->config_trusted) {
        char *argv[PROCESS_MAX_ARGS];
        char storage[1024];
        int argc = process_split_args(custom, argv, PROCESS_MAX_ARGS - 1, storage, sizeof(storage));
        if (argc > 0) {
            argv[argc] = (char *)path;
            argv[argc + 1] = NULL;
            if (app_launcher_spawn(argv) == 0) return 0;
        }
    }
    int native = platform_is_macos() || platform_wsl_interop();
    if (type == MESSAGE_TYPE_VIDEO && !native && open_video(path) == 0) return 0;
    if (app_launcher_open_path(path) == 0) return 0;
    LOG_WARN("no viewer available for %s", path);
    return -1;
}

static void opener_destroy(IMediaOpener *self) {
    free(self->ctx);
    free(self);
}

IMediaOpener *system_media_opener_create(const Settings *settings) {
    IMediaOpener *m = calloc(1, sizeof(*m));
    Opener *o = calloc(1, sizeof(*o));
    if (!m || !o) { free(m); free(o); return NULL; }
    o->settings = settings;
    m->ctx = o;
    m->open = opener_open;
    m->destroy = opener_destroy;
    return m;
}
