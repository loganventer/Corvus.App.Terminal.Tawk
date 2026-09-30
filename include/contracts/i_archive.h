#ifndef APP_CONTRACTS_I_ARCHIVE_H
#define APP_CONTRACTS_I_ARCHIVE_H

#include "core/archive_entry.h"

/* A compressed archive of files and folders. */
typedef struct IArchive {
    void *ctx;
    /* Packs `names` (relative to `root`) into `archive_path`. Returns 0 on success. */
    int  (*create)(struct IArchive *self, const char *root, const char *const *names, int count, const char *archive_path);
    /* Lists the members without extracting anything; returns how many were
     * written to `out` (at most `max`), or -1 when it cannot be read. */
    int  (*list)(struct IArchive *self, const char *archive_path, ArchiveEntry *out, int max);
    /* Extracts into `dest_dir` (which exists), never keeping the archive's
     * owners. Returns 0 on success. */
    int  (*extract)(struct IArchive *self, const char *archive_path, const char *dest_dir);
    void (*destroy)(struct IArchive *self);
} IArchive;

#endif
