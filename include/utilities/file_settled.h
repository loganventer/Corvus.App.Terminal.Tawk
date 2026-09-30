#ifndef APP_UTILITIES_FILE_SETTLED_H
#define APP_UTILITIES_FILE_SETTLED_H

/* True when `path` is a non-empty regular file that has not been written
 * for `quiet_ms`, so a program producing it in the background is done. */
int file_settled(const char *path, int quiet_ms);

#endif
