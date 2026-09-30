#ifndef APP_CORE_ARCHIVE_ENTRY_H
#define APP_CORE_ARCHIVE_ENTRY_H

/* One member of an archive, as listed before anything is extracted. */
typedef struct ArchiveEntry {
    char name[1024];
    char type;          /* '-' a file, 'd' a folder, anything else a link or special file */
} ArchiveEntry;

#endif
