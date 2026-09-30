/* Statuses over the control socket: who saw yours, posting one, and
 * answering other people's. */
#include "control_server_state.h"
#include "utilities/str_util.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>

#define MAX_VIEWERS 256

void control_op_status_viewers(ControlServer *s, ControlSession *session, const ControlRequest *req) {
    const char *id = control_required(s, session, req, "status_id");
    if (!id) return;
    StatusUpdate u;
    int found = s->deps.feed && status_feed_manager_get(s->deps.feed, id, &u) == 0;
    int mine = found && u.from_me;
    if (found) status_update_dispose(&u);
    if (!mine) {
        control_fail(s, session->conn, req->id, "not_found", "No status of yours has that id");
        return;
    }
    StatusViewer *viewers = calloc(MAX_VIEWERS, sizeof(*viewers));
    int n = viewers ? status_feed_manager_viewers(s->deps.feed, id, viewers, MAX_VIEWERS) : 0;
    cJSON *r = cJSON_CreateObject();
    cJSON *list = cJSON_AddArrayToObject(r, "viewers");
    for (int i = 0; i < n; i++) {
        char name[128];
        messaging_manager_display_name(s->deps.messaging, viewers[i].jid, name, sizeof(name));
        cJSON *o = cJSON_CreateObject();
        cJSON_AddStringToObject(o, "jid", viewers[i].jid);
        cJSON_AddStringToObject(o, "name", name);
        cJSON_AddNumberToObject(o, "ts", (double)viewers[i].viewed_at);
        cJSON_AddStringToObject(o, "liked", viewers[i].reaction);
        cJSON_AddItemToArray(list, o);
    }
    free(viewers);
    control_reply(s, session->conn, control_codec_ok(req->id, r));
}

void control_op_list_backgrounds(ControlServer *s, ControlSession *session, const ControlRequest *req) {
    cJSON *r = cJSON_CreateObject();
    cJSON *list = cJSON_AddArrayToObject(r, "backgrounds");
    int n = s->deps.statuses ? status_manager_background_count(s->deps.statuses) : 0;
    for (int i = 0; i < n; i++) cJSON_AddItemToArray(list, cJSON_CreateString(status_manager_background_name(s->deps.statuses, i)));
    control_reply(s, session->conn, control_codec_ok(req->id, r));
}

/* ---- post_status ---- */

static cJSON *do_post(ControlServer *s, const ControlPending *p, ControlFailure *f) {
    StatusPost post;
    memset(&post, 0, sizeof(post));
    post.kind = (StatusKind)control_codec_int(p->args, "kind", STATUS_KIND_TEXT, 0, STATUS_KIND_COUNT - 1);
    str_copy(post.text, sizeof(post.text), p->text ? p->text : "");
    const char *file = control_codec_string(p->args, "file");
    if (file) str_copy(post.path, sizeof(post.path), file);
    post.background_argb = status_manager_background(s->deps.statuses, (int)control_codec_int(p->args, "background", 0, 0, 1000));
    if (status_manager_post(s->deps.statuses, &post) != 0) {
        str_copy(f->why, sizeof(f->why), status_manager_error(s->deps.statuses));
        return NULL;
    }
    cJSON *r = cJSON_CreateObject();
    cJSON_AddStringToObject(r, "state", "posting");
    return r;
}

static int kind_named(const char *name, StatusKind *out) {
    static const char *const NAMES[STATUS_KIND_COUNT] = { "text", "photo", "video", "link" };
    for (int i = 0; name && i < STATUS_KIND_COUNT; i++) if (!strcmp(NAMES[i], name)) { *out = (StatusKind)i; return 0; }
    return -1;
}

void control_op_post_status(ControlServer *s, ControlSession *session, const ControlRequest *req) {
    if (!s->deps.statuses || !status_manager_supported(s->deps.statuses)) {
        control_fail(s, session->conn, req->id, "unsupported", "Posting a status needs the whatsmeow backend");
        return;
    }
    StatusKind kind;
    if (kind_named(control_codec_string(req->args, "kind"), &kind) != 0) {
        control_fail(s, session->conn, req->id, "bad_request", "\"kind\" is text, photo, video or link");
        return;
    }
    const char *text = control_codec_string(req->args, "text");
    const char *file = control_codec_string(req->args, "file");
    int media = kind == STATUS_KIND_PHOTO || kind == STATUS_KIND_VIDEO;
    if (media && (!file || !*file || status_manager_kind_for_file(s->deps.statuses, file) != kind)) {
        control_fail(s, session->conn, req->id, "bad_request", "\"file\" must be a photo or video on this computer, matching \"kind\"");
        return;
    }
    if (!media && (!text || !*text)) { control_fail(s, session->conn, req->id, "bad_request", "\"text\" is required"); return; }
    int background = 0;
    const char *colour = control_codec_string(req->args, "background");
    int colours = status_manager_background_count(s->deps.statuses);
    if (colour && *colour) {
        background = -1;
        for (int i = 0; i < colours; i++) if (!strcasecmp(status_manager_background_name(s->deps.statuses, i), colour)) background = i;
        if (background < 0) { control_fail(s, session->conn, req->id, "not_found", "No such background (see list_backgrounds)"); return; }
    } else if (colours > 0) {
        background = (int)(random() % colours);
    }
    ControlPending p;
    control_pending_init(&p, req->id, "post_status", WRITE_KIND_MANAGE, do_post);
    cJSON_AddNumberToObject(p.args, "kind", kind);
    cJSON_AddNumberToObject(p.args, "background", background);
    if (media) cJSON_AddStringToObject(p.args, "file", file);
    if (text) p.text = str_dup(text);
    p.editable = 1;
    p.needs_connection = 1;
    if (media) snprintf(p.action, sizeof(p.action), "post %s as your status: %.80s", kind == STATUS_KIND_PHOTO ? "a photo" : "a video", file);
    else snprintf(p.action, sizeof(p.action), "post a status on %s", colours ? status_manager_background_name(s->deps.statuses, background) : "a colour");
    control_write(s, session, &p);
}

/* ---- answering someone else's status ---- */

static int target_of(ControlServer *s, ControlSession *session, const ControlRequest *req, StatusReplyTarget *t) {
    const char *id = control_required(s, session, req, "status_id");
    if (!id) return -1;
    StatusUpdate u;
    int found = s->deps.feed && status_feed_manager_get(s->deps.feed, id, &u) == 0;
    if (!found || u.from_me) {
        if (found) status_update_dispose(&u);
        control_fail(s, session->conn, req->id, "not_found", "No status of someone else has that id");
        return -1;
    }
    memset(t, 0, sizeof(*t));
    str_copy(t->status_id, sizeof(t->status_id), u.id);
    str_copy(t->author_jid, sizeof(t->author_jid), u.author_jid);
    const char *kind = u.type == MESSAGE_TYPE_IMAGE ? "\xF0\x9F\x93\xB7 Photo" : u.type == MESSAGE_TYPE_VIDEO ? "\xF0\x9F\x8E\xAC Video" : "";
    str_copy(t->preview, sizeof(t->preview), u.text && u.text[0] ? u.text : kind);
    status_update_dispose(&u);
    const Chat *chat = control_visible_chat(s, t->author_jid);
    int n = 0;
    const Chat *all = messaging_manager_chats(s->deps.messaging, &n);
    int known = 0;
    for (int i = 0; i < n; i++) known |= !strcmp(all[i].jid, t->author_jid);
    if (known && !chat) {                            /* their chat is locked or not allowed */
        control_fail(s, session->conn, req->id, "not_found", "No status of someone else has that id");
        return -1;
    }
    return 0;
}

static void target_args(ControlPending *p, const StatusReplyTarget *t) {
    str_copy(p->chat_jid, sizeof(p->chat_jid), t->author_jid);
    cJSON_AddStringToObject(p->args, "status_id", t->status_id);
    cJSON_AddStringToObject(p->args, "author", t->author_jid);
    cJSON_AddStringToObject(p->args, "preview", t->preview);
}

static void target_from(const ControlPending *p, StatusReplyTarget *t) {
    memset(t, 0, sizeof(*t));
    str_copy(t->status_id, sizeof(t->status_id), control_codec_string(p->args, "status_id"));
    str_copy(t->author_jid, sizeof(t->author_jid), control_codec_string(p->args, "author"));
    str_copy(t->preview, sizeof(t->preview), control_codec_string(p->args, "preview"));
}

static cJSON *do_reply(ControlServer *s, const ControlPending *p, ControlFailure *f) {
    StatusReplyTarget t;
    target_from(p, &t);
    if (messaging_manager_reply_to_status(s->deps.messaging, &t, p->text) != 0) {
        str_copy(f->why, sizeof(f->why), "The reply could not be sent");
        return NULL;
    }
    return cJSON_CreateObject();
}

void control_op_reply_status(ControlServer *s, ControlSession *session, const ControlRequest *req) {
    const char *text = control_required(s, session, req, "text");
    if (!text) return;
    StatusReplyTarget t;
    if (target_of(s, session, req, &t) != 0) return;
    ControlPending p;
    control_pending_init(&p, req->id, "reply_status", WRITE_KIND_SEND, do_reply);
    target_args(&p, &t);
    p.text = str_dup(text);
    p.editable = 1;
    p.needs_connection = 1;
    snprintf(p.action, sizeof(p.action), "reply to their status \"%.60s\"", t.preview);
    control_write(s, session, &p);
}

static cJSON *do_like(ControlServer *s, const ControlPending *p, ControlFailure *f) {
    StatusReplyTarget t;
    target_from(p, &t);
    int how = messaging_manager_like_status(s->deps.messaging, &t);
    if (how < 0) { str_copy(f->why, sizeof(f->why), "The status could not be liked"); return NULL; }
    cJSON *r = cJSON_CreateObject();
    cJSON_AddStringToObject(r, "how", how == 0 ? "like" : "reply");
    return r;
}

void control_op_like_status(ControlServer *s, ControlSession *session, const ControlRequest *req) {
    StatusReplyTarget t;
    if (target_of(s, session, req, &t) != 0) return;
    ControlPending p;
    control_pending_init(&p, req->id, "like_status", WRITE_KIND_SEND, do_like);
    target_args(&p, &t);
    p.needs_connection = 1;
    snprintf(p.action, sizeof(p.action), "like their status \"%.60s\"", t.preview);
    control_write(s, session, &p);
}
