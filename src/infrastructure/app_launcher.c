#include "infrastructure/app_launcher.h"
#include "utilities/log.h"
#include "utilities/platform.h"
#include "utilities/process_util.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* /mnt/c/Users/x -> C:\Users\x ; /home/x -> \\wsl.localhost\<distro>\home\x */
static void to_windows_path(const char *path, char *out, size_t size) {
    if (strncmp(path, "/mnt/", 5) == 0 && path[5] && path[6] == '/') {
        snprintf(out, size, "%c:%s", path[5] - 32, path + 6);
    } else {
        const char *distro = getenv("WSL_DISTRO_NAME");
        snprintf(out, size, "\\\\wsl.localhost\\%s%s", distro ? distro : "Ubuntu", path);
    }
    for (char *p = out; *p; p++) if (*p == '/') *p = '\\';
}

int app_launcher_spawn(char *const argv[]) {
    if (process_spawn_detached(argv) != 0) return -1;
    LOG_DEBUG("opened with %s", argv[0]);
    return 0;
}

int app_launcher_try_tool(const char *tool, const char *arg) {
    if (!process_on_path(tool)) return -1;
    char *const argv[] = { (char *)tool, (char *)arg, NULL };
    return app_launcher_spawn(argv);
}

/* `target` is what every tool but explorer.exe is given; explorer.exe is given `windows_target`
 * (the Windows spelling of a path, or the address itself). */
static int open_default(const char *target, const char *windows_target) {
    if (platform_is_macos()) return app_launcher_try_tool("open", target);
    if (platform_wsl_interop()) {
        if (app_launcher_try_tool("wslview", target) == 0) return 0;
        if (app_launcher_try_tool("explorer.exe", windows_target) == 0) return 0;
    }
    if (app_launcher_try_tool("cygstart", target) == 0) return 0;
    if (app_launcher_try_tool("xdg-open", target) == 0) return 0;
    if (process_on_path("gio")) {
        char *const argv[] = { "gio", "open", (char *)target, NULL };
        return app_launcher_spawn(argv);
    }
    return -1;
}

int app_launcher_open_path(const char *path) {
    char windows[1024] = "";
    if (platform_wsl_interop()) to_windows_path(path, windows, sizeof(windows));
    return open_default(path, windows);
}

int app_launcher_open_url(const char *url) {
    return open_default(url, url);
}
