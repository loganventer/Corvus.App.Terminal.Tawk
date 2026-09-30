/* Managing messages waiting to be sent later. */
#include "control_server_state.h"
#include "utilities/clock_util.h"
#include "utilities/str_util.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

/* The waiting message `id`, when it is in a chat the client may see. */
static int find(ControlServer *s, ControlSession *session, const ControlRequest *req, ScheduledMessage *out) {
    const char *id = control_required(s, session, req, "id");
    if (!id) return -1;
    ScheduledMessage *items = NULL;
    int count = 0, found = -1;
    scheduling_manager_list(s->deps.scheduling, NULL, &items, &count);
    for (int i = 0; i < count && found < 0; i++) {
        if (!strcmp(items[i].id, id) && control_visible_chat(s, items[i].chat_jid)) found = i;
    }
    if (found >= 0) {
        *out = items[found];
        memset(&items[found], 0, sizeof(items[found]));
    }
    scheduled_message_array_free(items, count);
    if (found < 0) control_fail(s, session->conn, req->id, "not_found", "No message with that id is waiting");
    return found < 0 ? -1 : 0;
}

static void start(ControlPending *p, const ControlRequest *req, const char *op, WriteKind kind, ControlExecute run, const ScheduledMessage *m) {
    control_pending_init(p, req->id, op, kind, run);
    str_copy(p->chat_jid, sizeof(p->chat_jid), m->chat_jid);
    cJSON_AddStringToObject(p->args, "id", m->id);
    if (m->text) p->text = str_dup(m->text);
}

static cJSON *do_cancel(ControlServer *s, const ControlPending *p, ControlFailure *f) {
    if (scheduling_manager_cancel(s->deps.scheduling, control_codec_string(p->args, "id")) != 0) {
        str_copy(f->why, sizeof(f->why), "It had already gone, or was cancelled");
        return NULL;
    }
    return cJSON_CreateObject();
}

void control_op_cancel_scheduled(ControlServer *s, ControlSession *session, const ControlRequest *req) {
    ScheduledMessage m;
    if (find(s, session, req, &m) != 0) return;
    ControlPending p;
    start(&p, req, "cancel_scheduled", WRITE_KIND_DESTRUCTIVE, do_cancel, &m);
    str_copy(p.action, sizeof(p.action), "cancel this message waiting to be sent");
    scheduled_message_dispose(&m);
    control_write(s, session, &p);
}

static cJSON *do_reschedule(ControlServer *s, const ControlPending *p, ControlFailure *f) {
    int64_t due = (int64_t)control_codec_int(p->args, "due_at", 0, 0, INT64_MAX);
    if (scheduling_manager_reschedule(s->deps.scheduling, control_codec_string(p->args, "id"), due, (int64_t)time(NULL)) != 0) {
        str_copy(f->why, sizeof(f->why), scheduling_manager_error(s->deps.scheduling));
        return NULL;
    }
    cJSON *r = cJSON_CreateObject();
    cJSON_AddNumberToObject(r, "due_at", (double)due);
    return r;
}

void control_op_reschedule(ControlServer *s, ControlSession *session, const ControlRequest *req) {
    int64_t due = 0;
    if (scheduling_manager_parse_when(s->deps.scheduling, control_codec_string(req->args, "when"), (int64_t)time(NULL), &due) != 0) {
        control_fail(s, session->conn, req->id, "bad_request", scheduling_manager_error(s->deps.scheduling));
        return;
    }
    ScheduledMessage m;
    if (find(s, session, req, &m) != 0) return;
    ControlPending p;
    start(&p, req, "reschedule", WRITE_KIND_MANAGE, do_reschedule, &m);
    cJSON_AddNumberToObject(p.args, "due_at", (double)due);
    char when[48];
    clock_format_upcoming(due, control_settings(s)->use_24h_clock, when, sizeof(when));
    snprintf(p.action, sizeof(p.action), "move this message to %s", when);
    scheduled_message_dispose(&m);
    control_write(s, session, &p);
}

static cJSON *do_now(ControlServer *s, const ControlPending *p, ControlFailure *f) {
    if (scheduling_manager_send_now(s->deps.scheduling, control_codec_string(p->args, "id"), (int64_t)time(NULL)) != 0) {
        str_copy(f->why, sizeof(f->why), "It had already gone, or was cancelled");
        return NULL;
    }
    return cJSON_CreateObject();
}

void control_op_send_scheduled_now(ControlServer *s, ControlSession *session, const ControlRequest *req) {
    ScheduledMessage m;
    if (find(s, session, req, &m) != 0) return;
    ControlPending p;
    start(&p, req, "send_scheduled_now", WRITE_KIND_SEND, do_now, &m);
    str_copy(p.action, sizeof(p.action), "send this waiting message now");
    scheduled_message_dispose(&m);
    control_write(s, session, &p);
}
