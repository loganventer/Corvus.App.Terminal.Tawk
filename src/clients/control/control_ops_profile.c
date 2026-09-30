/* Your own profile over the control socket: name, about text and photo. */
#include "control_server_state.h"
#include "utilities/path_util.h"
#include "utilities/str_util.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void control_op_get_profile(ControlServer *s, ControlSession *session, const ControlRequest *req) {
    cJSON *r = cJSON_CreateObject();
    cJSON_AddStringToObject(r, "jid", messaging_manager_user_jid(s->deps.messaging));
    cJSON_AddStringToObject(r, "name", messaging_manager_user_name(s->deps.messaging));
    control_reply(s, session->conn, control_codec_ok(req->id, r));
}

static cJSON *do_set_profile(ControlServer *s, const ControlPending *p, ControlFailure *f) {
    const char *name = control_codec_string(p->args, "name");
    int about = control_codec_bool(p->args, "about", 0);
    if (name && account_manager_set_name(s->deps.accounts, name) != 0) {
        str_copy(f->why, sizeof(f->why), account_manager_error(s->deps.accounts));
        return NULL;
    }
    if (about && account_manager_set_about(s->deps.accounts, p->text ? p->text : "") != 0) {
        str_copy(f->why, sizeof(f->why), account_manager_error(s->deps.accounts));
        return NULL;
    }
    return cJSON_CreateObject();
}

void control_op_set_profile(ControlServer *s, ControlSession *session, const ControlRequest *req) {
    const char *name = control_codec_string(req->args, "name");
    const char *about = control_codec_string(req->args, "about");
    if ((!name || !*name) && !about) { control_fail(s, session->conn, req->id, "bad_request", "Give \"name\", \"about\" or both"); return; }
    if (!s->deps.accounts) { control_fail(s, session->conn, req->id, "unsupported", "This backend cannot change your profile"); return; }
    ControlPending p;
    control_pending_init(&p, req->id, "set_profile", WRITE_KIND_MANAGE, do_set_profile);
    if (name && *name) cJSON_AddStringToObject(p.args, "name", name);
    if (about) {
        cJSON_AddBoolToObject(p.args, "about", 1);
        p.text = str_dup(about);
        p.editable = 1;
    }
    if (name && *name && about) snprintf(p.action, sizeof(p.action), "change your name to \"%.40s\" and your about text to", name);
    else if (name && *name) snprintf(p.action, sizeof(p.action), "change your name to \"%.60s\"", name);
    else str_copy(p.action, sizeof(p.action), "change your about text to");
    p.needs_connection = 1;
    control_write(s, session, &p);
}

static cJSON *do_photo(ControlServer *s, const ControlPending *p, ControlFailure *f) {
    const char *file = control_codec_string(p->args, "file");
    int rc = file ? account_manager_set_picture(s->deps.accounts, file) : account_manager_remove_picture(s->deps.accounts);
    if (rc != 0) { str_copy(f->why, sizeof(f->why), account_manager_error(s->deps.accounts)); return NULL; }
    return cJSON_CreateObject();
}

void control_op_set_profile_photo(ControlServer *s, ControlSession *session, const ControlRequest *req) {
    const char *file = control_required(s, session, req, "file");
    if (!file) return;
    if (!s->deps.accounts) { control_fail(s, session->conn, req->id, "unsupported", "This backend cannot change your profile"); return; }
    if (!path_is_regular_file(file)) { control_fail(s, session->conn, req->id, "not_found", "No such file"); return; }
    ControlPending p;
    control_pending_init(&p, req->id, "set_profile_photo", WRITE_KIND_MANAGE, do_photo);
    cJSON_AddStringToObject(p.args, "file", file);
    snprintf(p.action, sizeof(p.action), "make %.90s your profile photo", file);
    p.needs_connection = 1;
    control_write(s, session, &p);
}

void control_op_remove_profile_photo(ControlServer *s, ControlSession *session, const ControlRequest *req) {
    if (!s->deps.accounts) { control_fail(s, session->conn, req->id, "unsupported", "This backend cannot change your profile"); return; }
    ControlPending p;
    control_pending_init(&p, req->id, "remove_profile_photo", WRITE_KIND_DESTRUCTIVE, do_photo);
    str_copy(p.action, sizeof(p.action), "remove your profile photo");
    p.needs_connection = 1;
    control_write(s, session, &p);
}
