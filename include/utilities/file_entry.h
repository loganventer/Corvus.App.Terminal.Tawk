#ifndef APP_UTILITIES_FILE_ENTRY_H
#define APP_UTILITIES_FILE_ENTRY_H

/* One item in a directory listing. */
typedef struct FileEntry {
    char      name[256];
    int       is_dir;
    long long size;
} FileEntry;

#endif
