#ifndef APP_CLIENTS_CONTROL_CONTROL_CODEC_H
#define APP_CLIENTS_CONTROL_CONTROL_CODEC_H

#include <stdint.h>

#include "cJSON.h"
#include "clients/control/control_request.h"
#include "core/chat.h"
#include "core/message.h"

/* Turns control socket lines into requests and tawk's values into the JSON
 * described in CONTROL.md. Every returned string is one line, without the
 * newline; the caller frees it. */

/* Returns 0, or -1 when the line is not a request (with the id, if any, kept). */
int    control_codec_parse(const char *line, ControlRequest *out);
cJSON *control_codec_chat(const Chat *chat);
/* `sender_name` is what to call the sender ("You" for your own). */
cJSON *control_codec_message(const Message *msg, const char *sender_name);
/* Take ownership of `result` / `extra`, which may be NULL. */
char  *control_codec_ok(const char *id, cJSON *result);
char  *control_codec_error(const char *id, const char *code, const char *message, cJSON *extra);
char  *control_codec_event(const char *evt, cJSON *fields);

/* Argument readers: the value, or `fallback` when missing or of another type. */
const char *control_codec_string(const cJSON *args, const char *name);
int64_t     control_codec_int(const cJSON *args, const char *name, int64_t fallback, int64_t min, int64_t max);
int         control_codec_bool(const cJSON *args, const char *name, int fallback);

#endif
