#ifndef APP_RESOURCE_ACCESS_JSON_PROTOCOL_H
#define APP_RESOURCE_ACCESS_JSON_PROTOCOL_H

#include "core/delete_request.h"
#include "core/event.h"
#include "core/history_anchor.h"
#include "core/outgoing_media.h"
#include "core/outgoing_text.h"
#include "core/quote_ref.h"
#include "core/reaction_target.h"
#include "core/read_request.h"
#include "core/status_post.h"

/* The line protocol spoken by every backend (Node sidecar and in-process
 * whatsmeow). One JSON object per line; see docs/protocol.md.
 * All decoded strings are bounded and single-line fields have control
 * characters removed, so remote data cannot inject terminal sequences. */

/* Returns 0 and fills *out, or -1 for malformed or unknown input. */
int   json_protocol_decode(const char *line, Event *out);

/* Encoders return a malloc'd JSON object without trailing newline. */
char *json_protocol_encode_connect(void);
char *json_protocol_encode_reconnect(void);
char *json_protocol_encode_send(const char *jid, const OutgoingText *text, const char *message_id);
char *json_protocol_encode_edit(const char *jid, const char *message_id, const char *text);
char *json_protocol_encode_delete(const DeleteRequest *request);
/* Deletes a whole chat; request->id and ->timestamp name its newest message. */
char *json_protocol_encode_delete_chat(const DeleteRequest *request);
char *json_protocol_encode_profile(const char *jid);
char *json_protocol_encode_reject_call(const char *from, const char *call_id);
char *json_protocol_encode_picture(const char *jid, int full);
char *json_protocol_encode_block(const char *jid, int block);
char *json_protocol_encode_react(const ReactionTarget *target, const char *emoji);
/* state: composing, recording or paused */
char *json_protocol_encode_typing(const char *jid, const char *state);
char *json_protocol_encode_subscribe(const char *jid);
char *json_protocol_encode_presence(int available);
char *json_protocol_encode_history(const HistoryAnchor *anchor, int count);
/* Voice note: an Ogg/Opus file inside the media folder. */
char *json_protocol_encode_send_voice(const char *jid, const char *path, int seconds, const char *message_id);
/* Photo, video, audio or document inside the media folder. */
char *json_protocol_encode_send_media(const char *jid, const OutgoingMedia *media, const char *message_id);
char *json_protocol_encode_forward_media(const char *jid, const char *media_ref, int forwarding_score, const char *message_id);
char *json_protocol_encode_pair(const char *phone_digits);
char *json_protocol_encode_qr(void);
char *json_protocol_encode_download(const char *message_id, const char *media_ref, int max_mb);
char *json_protocol_encode_read(const ReadRequest *request);
char *json_protocol_encode_like_status(const char *author, const char *status_id, const char *emoji);
char *json_protocol_encode_logout(void);
/* Your own profile. The picture is a JPEG or PNG inside the media folder. */
char *json_protocol_encode_set_name(const char *name);
char *json_protocol_encode_set_about(const char *text);
char *json_protocol_encode_set_picture(const char *path);
char *json_protocol_encode_remove_picture(void);
/* A status to status@broadcast; media paths are inside the media folder. */
char *json_protocol_encode_post_status(const StatusPost *post);
/* Start-up configuration for in-process backends. */
char *json_protocol_encode_init(const char *auth_dir, const char *media_dir, const char *log_dir, int debug);

#endif
