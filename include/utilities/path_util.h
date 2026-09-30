#ifndef APP_UTILITIES_PATH_UTIL_H
#define APP_UTILITIES_PATH_UTIL_H

#include <stddef.h>

/* $XDG_CONFIG_HOME/APP_NAME or ~/.config/APP_NAME */
void path_config_dir(char *out, size_t size);
/* $XDG_DATA_HOME/APP_NAME or ~/.local/share/APP_NAME */
void path_data_dir(char *out, size_t size);
/* $XDG_CACHE_HOME/APP_NAME or ~/.cache/APP_NAME (re-downloadable media) */
void path_cache_dir(char *out, size_t size);
/* $XDG_STATE_HOME/APP_NAME or ~/.local/state/APP_NAME (logs) */
void path_state_dir(char *out, size_t size);
/* $XDG_RUNTIME_DIR/APP_NAME, or the state folder where there is none (sockets). */
void path_runtime_dir(char *out, size_t size);
/* Expands a leading "~/" to $HOME. No other expansion is performed, so a
 * config value can never trigger command substitution. */
void path_expand_home(const char *in, char *out, size_t size);
/* The user's downloads folder: XDG_DOWNLOAD_DIR (from the environment or
 * ~/.config/user-dirs.dirs), else ~/Downloads. */
void path_download_dir(char *out, size_t size);
/* Creates the directory and its parents with the given mode. */
int  path_mkdir_p(const char *path, unsigned mode);
/* Joins dir and name with a single slash. */
void path_join(char *out, size_t size, const char *dir, const char *name);
int  path_is_regular_file(const char *path);
/* True when the resolved path lies inside the resolved directory. */
int  path_is_within(const char *path, const char *dir);

#endif
