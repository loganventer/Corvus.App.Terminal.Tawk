#ifndef APP_ENGINES_STATUS_POST_VALIDATOR_H
#define APP_ENGINES_STATUS_POST_VALIDATOR_H

#include <stddef.h>

#include "core/status_post.h"

#define STATUS_POST_MAX_MEDIA_BYTES (100L * 1024 * 1024)

/* 0 when the status can be posted: text statuses need words, link statuses
 * a web address, photos and videos an existing file of that type (at most
 * 100 MB). Otherwise -1 and a reason for people in `why`. */
int status_post_validate(const StatusPost *post, char *why, size_t why_size);

#endif
