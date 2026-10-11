#ifndef APP_UTILITIES_FILE_FACTS_H
#define APP_UTILITIES_FILE_FACTS_H

/* What the file system says about one path, with nothing decided about it. */
typedef struct FileFacts {
    int       exists;
    int       is_regular;     /* a plain file, not a folder, device or socket */
    long long size_bytes;
} FileFacts;

#endif
