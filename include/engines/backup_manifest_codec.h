#ifndef APP_ENGINES_BACKUP_MANIFEST_CODEC_H
#define APP_ENGINES_BACKUP_MANIFEST_CODEC_H

#include <stddef.h>

#include "core/backup_manifest.h"

/* manifest.txt: "key=value" lines, readable by people. */
int backup_manifest_format(const BackupManifest *manifest, char *out, size_t size);
/* Returns 0 when `text` is a manifest this version can restore (a known
 * format number); unknown keys are ignored. */
int backup_manifest_parse(const char *text, BackupManifest *out);

#endif
