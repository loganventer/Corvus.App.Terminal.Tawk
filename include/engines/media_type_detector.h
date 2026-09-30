#ifndef APP_ENGINES_MEDIA_TYPE_DETECTOR_H
#define APP_ENGINES_MEDIA_TYPE_DETECTOR_H

#include "core/message_type.h"

/* Classifies a file for sending by its extension: photos and videos are sent
 * as media, audio files as audio, everything else as a document. */
MessageType media_type_detect(const char *path);
/* MIME type for the file, e.g. "image/jpeg"; "application/octet-stream" when unknown. */
const char *media_type_mime(const char *path);
/* True when tawk knows the file type (by extension) and can show or play it. */
int         media_type_known(const char *path);
/* Lower-case extension including the dot, or "". */
const char *media_type_extension(const char *path);

#endif
