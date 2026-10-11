/* Sending a file over the control socket: a photo, video, audio file or
 * document from this computer, with an optional caption. */
#include "control_server_state.h"
#include "engines/byte_size_text.h"
#include "managers/attachment_inspection.h"
#include "utilities/str_util.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define CAPTION_MAX_BYTES 1024

static cJSON *do_send_file(ControlServer *s, const ControlPending *p, ControlFailure *f) {
    const char *file = control_codec_string(p->args, "file");
    AttachmentInfo info;
    /* Looked at again now, after the wait: the file may have changed or been replaced meanwhile. */
    AttachmentVerdict verdict = attachment_inspection_run(control_settings(s), file, &info);
    if (verdict != ATTACHMENT_OK) {
        str_copy(f->why, sizeof(f->why), attachment_verdict_reason(verdict));
        return NULL;
    }
    uint64_t before = messaging_manager_live_last(s->deps.messaging);
    if (messaging_manager_send_file_to(s->deps.messaging, p->chat_jid, file, p->text) != 0) {
        str_copy(f->why, sizeof(f->why), "The file could not be queued");
        return NULL;
    }
    cJSON *r = cJSON_CreateObject();
    LiveMessageRef sent[4];
    int n = messaging_manager_live_since(s->deps.messaging, before, sent, 4);
    for (int i = 0; i < n; i++) {
        if (strcmp(sent[i].chat_jid, p->chat_jid) == 0) { cJSON_AddStringToObject(r, "id", sent[i].id); break; }
    }
    cJSON_AddStringToObject(r, "file", info.name);
    cJSON_AddStringToObject(r, "type", info.kind ? info.kind : "document");
    cJSON_AddNumberToObject(r, "bytes", (double)info.size_bytes);
    return r;
}

void control_op_send_file(ControlServer *s, ControlSession *session, const ControlRequest *req) {
    if (control_follow_sender(s, session, req) != 0) return;
    char jid[128];
    int new_chat = 0;
    if (control_resolve_recipient(s, session, req, "chat", jid, sizeof(jid), &new_chat) != 0) return;
    const char *file = control_required(s, session, req, "file");
    if (!file) return;
    const char *caption = control_codec_string(req->args, "caption");
    if (caption && strlen(caption) > CAPTION_MAX_BYTES) {
        control_fail(s, session->conn, req->id, "bad_request", "\"caption\" is longer than 1024 bytes");
        return;
    }
    AttachmentInfo info;
    AttachmentVerdict verdict = attachment_inspection_run(control_settings(s), file, &info);
    if (verdict != ATTACHMENT_OK) {
        control_fail(s, session->conn, req->id, verdict == ATTACHMENT_NOT_FOUND ? "not_found" : "bad_request",
                     attachment_verdict_reason(verdict));
        return;
    }
    ControlPending p;
    control_pending_init(&p, req->id, "send_file", WRITE_KIND_SEND, do_send_file);
    str_copy(p.chat_jid, sizeof(p.chat_jid), jid);
    p.new_chat = new_chat;
    cJSON_AddStringToObject(p.args, "file", file);
    /* An empty caption still goes in as "", so the AI disclaimer, when one is set, has words to stand under. */
    p.text = str_dup(caption ? caption : "");
    p.editable = caption && *caption;
    p.needs_connection = 1;
    char size[32];
    byte_size_text(info.size_bytes, size, sizeof(size));
    snprintf(p.action, sizeof(p.action), "%s \"%.60s\" (%s, %s)",
             new_chat ? "start a new chat by sending the file" : "send the file", info.name, info.kind ? info.kind : "document", size);
    control_write(s, session, &p);
}
