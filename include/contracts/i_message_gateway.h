#ifndef APP_CONTRACTS_I_MESSAGE_GATEWAY_H
#define APP_CONTRACTS_I_MESSAGE_GATEWAY_H

/* Transport to the WhatsApp network. Implementations push decoded events
 * into the event queue they were constructed with. They never reconnect on
 * their own: they report EVENT_CONNECTION_STATUS with a reason and the
 * messaging manager decides when to call connect (backoff, circuit breaker). */
#include "core/delete_request.h"
#include "core/history_anchor.h"
#include "core/outgoing_media.h"
#include "core/outgoing_text.h"
#include "core/quote_ref.h"
#include "core/read_request.h"
#include "core/reaction_target.h"

typedef struct IMessageGateway {
    void *ctx;
    /* Starts the backend runtime (process or library). */
    int  (*start)(struct IMessageGateway *self);
    /* Opens the WhatsApp connection; emits QR/pairing events when not linked. */
    int  (*connect)(struct IMessageGateway *self);
    /* Drops the current connection, even one that still looks open, and
     * connects again. Used when the machine's network changes. */
    int  (*reconnect)(struct IMessageGateway *self);
    void (*stop)(struct IMessageGateway *self);
    /* quote may be NULL; otherwise the message is sent as a reply. */
    int  (*send_text)(struct IMessageGateway *self, const char *jid, const OutgoingText *text, const char *message_id);
    /* Replaces the text of one of our messages. */
    int  (*edit)(struct IMessageGateway *self, const char *jid, const char *message_id, const char *text);
    /* Deletes a message for everyone (revoke) or only for this account. */
    int  (*delete_message)(struct IMessageGateway *self, const DeleteRequest *request);
    /* Deletes a whole chat on every device; the request names its newest message. */
    int  (*delete_chat)(struct IMessageGateway *self, const DeleteRequest *request);
    /* Asks for a contact's or group's details (answered by a profile event). */
    int  (*request_profile)(struct IMessageGateway *self, const char *jid);
    /* Asks for a profile picture, the preview or the full size (answered by a picture event). */
    int  (*request_picture)(struct IMessageGateway *self, const char *jid, int full);
    int  (*set_blocked)(struct IMessageGateway *self, const char *jid, int blocked);
    /* Declines an incoming call. */
    int  (*reject_call)(struct IMessageGateway *self, const char *from, const char *call_id);
    /* An empty emoji removes our reaction. */
    int  (*react)(struct IMessageGateway *self, const ReactionTarget *target, const char *emoji);
    /* Our chat state in `jid`: composing, recording or paused. */
    int  (*typing)(struct IMessageGateway *self, const char *jid, const char *state);
    /* Ask for typing and online updates for a chat. */
    int  (*subscribe)(struct IMessageGateway *self, const char *jid);
    /* Asks the phone for up to `count` messages older than the anchor; they
     * arrive later as history messages. */
    int  (*request_older)(struct IMessageGateway *self, const HistoryAnchor *anchor, int count);
    /* Appear online (1) or offline (0) to contacts. */
    int  (*presence)(struct IMessageGateway *self, int available);
    /* Sends an Ogg/Opus file (inside the media folder) as a voice note. */
    int  (*send_voice)(struct IMessageGateway *self, const char *jid, const char *path, int seconds, const char *message_id);
    /* Sends a photo, video, audio file or document (inside the media folder). */
    int  (*send_media)(struct IMessageGateway *self, const char *jid, const OutgoingMedia *media, const char *message_id);
    /* Sends media WhatsApp already has (from a received message's download
     * reference) to another chat, marked as forwarded, without uploading it again. */
    int  (*forward_media)(struct IMessageGateway *self, const char *jid, const char *media_ref, int forwarding_score,
                          const char *message_id);
    int  (*request_pairing_code)(struct IMessageGateway *self, const char *phone_digits);
    int  (*request_qr)(struct IMessageGateway *self);
    int  (*download_media)(struct IMessageGateway *self, const char *message_id, const char *media_ref, int max_mb);
    /* Marks a chat as read: read receipts when asked, and the read mark that
     * clears its unread badge on your other devices. */
    int  (*mark_read)(struct IMessageGateway *self, const ReadRequest *request);
    int  (*logout)(struct IMessageGateway *self);
    void (*destroy)(struct IMessageGateway *self);
} IMessageGateway;

#endif
