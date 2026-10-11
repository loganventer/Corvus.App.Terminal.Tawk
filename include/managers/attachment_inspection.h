#ifndef APP_MANAGERS_ATTACHMENT_INSPECTION_H
#define APP_MANAGERS_ATTACHMENT_INSPECTION_H

#include "core/attachment_info.h"
#include "core/attachment_verdict.h"
#include "core/settings.h"

/* The most an agent may send as one file. */
#define ATTACHMENT_MAX_BYTES (100LL * 1024 * 1024)

/* Looks at a file an agent wants sent. Fills `info` with what it is (its name,
 * kind and size) when it exists, and says whether it may go: nothing from
 * tawk's own login, database or settings, nothing from the places that hold
 * keys and credentials, no folders, no empty files, nothing past
 * ATTACHMENT_MAX_BYTES. Reads the file system; changes nothing. */
AttachmentVerdict attachment_inspection_run(const Settings *settings, const char *path, AttachmentInfo *info);

#endif
