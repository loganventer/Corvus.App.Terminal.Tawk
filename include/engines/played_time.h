#ifndef APP_ENGINES_PLAYED_TIME_H
#define APP_ENGINES_PLAYED_TIME_H

#include <stdint.h>

#include "core/message_type.h"
#include "core/receipt.h"

/* Whether a message of this type is one a recipient can play: a voice note or a video. */
int played_time_applies(MessageType type);

/* When a voice note or video you sent was first played by anyone: the earliest
 * played time among its receipts, or 0 when nobody has played it. */
int64_t played_time_first(const Receipt *receipts, int count);

#endif
