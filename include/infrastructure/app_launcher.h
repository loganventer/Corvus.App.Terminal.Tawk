#ifndef APP_INFRASTRUCTURE_APP_LAUNCHER_H
#define APP_INFRASTRUCTURE_APP_LAUNCHER_H

/* Starts programs for the user without waiting: the platform's default application for a file or a web
 * address (open on macOS, wslview or explorer.exe on WSL with interop, cygstart on MSYS2/Cygwin,
 * xdg-open or gio on Linux). Every call returns 0 when something was started, -1 otherwise. */

/* Starts argv detached. */
int app_launcher_spawn(char *const argv[]);
/* Starts `tool` with one argument, when `tool` is on PATH. */
int app_launcher_try_tool(const char *tool, const char *arg);
/* Opens a file in its default application. */
int app_launcher_open_path(const char *path);
/* Opens an address in the default browser. */
int app_launcher_open_url(const char *url);

#endif
