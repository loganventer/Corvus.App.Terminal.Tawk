#ifndef APP_CORE_MEDIA_PATH_MAP_H
#define APP_CORE_MEDIA_PATH_MAP_H

/* How a path to a downloaded file changes when the account moves: the media
 * folder it was in, the one it goes to, and whether the files came along. */
typedef struct MediaPathMap {
    const char *old_dir;
    const char *new_dir;
    int         keep;           /* 0: the files stayed behind, so the paths are cleared */
} MediaPathMap;

#endif
