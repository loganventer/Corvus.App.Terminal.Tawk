#ifndef APP_UTILITIES_OUTGOING_MEDIA_H
#define APP_UTILITIES_OUTGOING_MEDIA_H

#include <stddef.h>

/* Backends only send files inside the media folder, so a file to send is
 * first copied to <media_dir>/outgoing/<id><extension>. Writes the copy's
 * path to `out`; returns 0 on success, -1 otherwise. */
int outgoing_media_copy(const char *media_dir, const char *source, const char *id, char *out, size_t out_size);

#endif
