/* Live updates: clients that subscribe hear about new messages in their
 * chats, and about unread counts that change. */
#include "control_server_state.h"
#include "utilities/str_util.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define LIVE_PER_TICK 64

static const Chat *find_chat(ControlServer *s, const char *jid) {
    int n = 0;
    const Chat *all = messaging_manager_chats(s->deps.messaging, &n);
    for (int i = 0; i < n; i++) if (strcmp(all[i].jid, jid) == 0) return &all[i];
    return NULL;
}

/* Remembers the unread counts the client has now, so only changes are sent. */
static void snapshot(ControlServer *s, ControlSession *session) {
    int n = 0;
    const Chat *all = messaging_manager_chats(s->deps.messaging, &n);
    free(session->watches);
    session->watches = n ? calloc((size_t)n, sizeof(ControlWatch)) : NULL;
    session->watch_count = 0;
    for (int i = 0; i < n && session->watches; i++) {
        if (!control_session_follows(session, all[i].jid) || !automation_manager_chat_allowed(s->deps.automation, &all[i])) continue;
        ControlWatch *w = &session->watches[session->watch_count++];
        str_copy(w->jid, sizeof(w->jid), all[i].jid);
        w->unread = all[i].unread;
    }
}

void control_op_subscribe(ControlServer *s, ControlSession *session, const ControlRequest *req) {
    const cJSON *chats = cJSON_GetObjectItemCaseSensitive(req->args, "chats");
    session->all_chats = 0;
    session->chat_count = 0;
    if (cJSON_IsString(chats) && strcmp(chats->valuestring, "all") == 0) {
        session->all_chats = 1;
    } else if (cJSON_IsArray(chats)) {
        int n = 0;
        const Chat *all = messaging_manager_chats(s->deps.messaging, &n);
        const cJSON *item;
        cJSON_ArrayForEach(item, chats) {
            if (!cJSON_IsString(item) || session->chat_count >= CONTROL_SESSION_CHATS) continue;
            int found = -1;
            if (automation_manager_resolve(s->deps.automation, all, n, item->valuestring, &found, NULL, 0, NULL) != CHAT_RESOLUTION_FOUND) {
                char why[256];
                snprintf(why, sizeof(why), "\"%s\" is not one chat you may see", item->valuestring);
                control_fail(s, session->conn, req->id, "not_found", why);
                return;
            }
            str_copy(session->chats[session->chat_count++], sizeof(session->chats[0]), all[found].jid);
        }
    } else {
        control_fail(s, session->conn, req->id, "bad_request", "\"chats\" is a list of chats, or \"all\"");
        return;
    }
    session->subscribed = 1;
    snapshot(s, session);
    control_reply(s, session->conn, control_codec_ok(req->id, NULL));
}

void control_op_unsubscribe(ControlServer *s, ControlSession *session, const ControlRequest *req) {
    session->subscribed = 0;
    session->all_chats = 0;
    session->chat_count = 0;
    control_session_dispose(session);
    control_reply(s, session->conn, control_codec_ok(req->id, NULL));
}

static void send_message(ControlServer *s, const LiveMessageRef *ref) {
    const Chat *chat = find_chat(s, ref->chat_jid);
    if (!chat || !automation_manager_chat_allowed(s->deps.automation, chat)) return;
    Message msg;
    int loaded = 0;
    for (int i = 0; i < s->session_count; i++) {
        ControlSession *session = &s->sessions[i];
        if (!session->greeted || !control_session_follows(session, ref->chat_jid)) continue;
        if (!loaded) {
            if (messaging_manager_get(s->deps.messaging, ref->id, &msg) != 0) return;
            loaded = 1;
        }
        char name[128];
        control_sender_name(s, &msg, name, sizeof(name));
        cJSON *evt = cJSON_CreateObject();
        cJSON *c = cJSON_AddObjectToObject(evt, "chat");
        cJSON_AddStringToObject(c, "jid", chat->jid);
        cJSON_AddStringToObject(c, "name", chat->name);
        cJSON_AddItemToObject(evt, "message", control_codec_message(&msg, name));
        control_reply(s, session->conn, control_codec_event("message", evt));
    }
    if (loaded) message_dispose(&msg);
}

static void send_unread_changes(ControlServer *s, ControlSession *session) {
    int n = 0;
    const Chat *all = messaging_manager_chats(s->deps.messaging, &n);
    for (int i = 0; i < n; i++) {
        if (!control_session_follows(session, all[i].jid)) continue;
        ControlWatch *w = NULL;
        for (int k = 0; k < session->watch_count && !w; k++) if (!strcmp(session->watches[k].jid, all[i].jid)) w = &session->watches[k];
        int allowed = automation_manager_chat_allowed(s->deps.automation, &all[i]);
        if (!allowed) continue;
        if (w && w->unread == all[i].unread) continue;
        if (!w && all[i].unread == 0) continue;
        cJSON *evt = cJSON_CreateObject();
        cJSON_AddItemToObject(evt, "chat", control_codec_chat(&all[i]));
        control_reply(s, session->conn, control_codec_event("chat", evt));
        if (!w) { snapshot(s, session); return; }         /* a chat it had not seen: start over */
        w->unread = all[i].unread;
    }
}

void control_live_tick(ControlServer *s, int check_unread) {
    LiveMessageRef refs[LIVE_PER_TICK];
    int n = messaging_manager_live_since(s->deps.messaging, s->live_seq, refs, LIVE_PER_TICK);
    for (int i = 0; i < n; i++) {
        send_message(s, &refs[i]);
        s->live_seq = refs[i].seq;
    }
    if (!check_unread) return;
    for (int i = 0; i < s->session_count; i++) {
        if (s->sessions[i].greeted && s->sessions[i].subscribed) send_unread_changes(s, &s->sessions[i]);
    }
}
