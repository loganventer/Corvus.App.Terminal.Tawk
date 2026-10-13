#ifndef APP_ENGINES_RECEIPT_ELAPSED_TEXT_H
#define APP_ENGINES_RECEIPT_ELAPSED_TEXT_H

#include <stddef.h>
#include <stdint.h>

/* How long after a message was sent something happened to it, in words:
 * "41 s after it was sent", "4 min 12 s after it was sent",
 * "2 h 57 min after it was sent", "3 d 2 h after it was sent". Empty when
 * the time is not after the send, such as a clock a little behind. */
void receipt_elapsed_text(int64_t sent_at, int64_t at, char *out, size_t size);

#endif
