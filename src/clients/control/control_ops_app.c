/* tawk itself over the control socket: settings, themes, the connection
 * and a ringing call. */
#include "control_server_state.h"
#include "core/settings_schema.h"
#include "utilities/app_info.h"
#include "utilities/str_util.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static const char *const KIND_NAMES[] = { "bool", "int", "string", "theme", "choice" };

static const SettingField *field_named(const char *section, const char *key) {
    for (int c = 0; section && c < SETTING_CATEGORY_COUNT; c++) {
        if (strcmp(setting_category_section((SettingCategory)c), section) == 0) return settings_schema_find((SettingCategory)c, key);
    }
    return NULL;
}

void control_op_get_settings(ControlServer *s, ControlSession *session, const ControlRequest *req) {
    const Settings *now = control_settings(s);
    cJSON *r = cJSON_CreateObject();
    cJSON *list = cJSON_AddArrayToObject(r, "settings");
    for (int i = 0; i < settings_schema_count(); i++) {
        const SettingField *f = settings_schema_at(i);
        char value[600];
        setting_to_text(now, f, value, sizeof(value));
        cJSON *o = cJSON_CreateObject();
        cJSON_AddStringToObject(o, "section", setting_category_section(f->category));
        cJSON_AddStringToObject(o, "key", f->key);
        cJSON_AddStringToObject(o, "label", f->label);
        cJSON_AddStringToObject(o, "help", f->help);
        cJSON_AddStringToObject(o, "kind", f->kind <= SETTING_KIND_CHOICE ? KIND_NAMES[f->kind] : "string");
        cJSON_AddStringToObject(o, "value", value);
        if (f->choices) cJSON_AddStringToObject(o, "choices", f->choices);
        if (f->kind == SETTING_KIND_INT) {
            cJSON_AddNumberToObject(o, "min", f->min);
            cJSON_AddNumberToObject(o, "max", f->max);
        }
        cJSON_AddBoolToObject(o, "changeable", automation_manager_setting_changeable(s->deps.automation, f));
        cJSON_AddItemToArray(list, o);
    }
    control_reply(s, session->conn, control_codec_ok(req->id, r));
}

static cJSON *do_set(ControlServer *s, const ControlPending *p, ControlFailure *f) {
    const SettingField *field = field_named(control_codec_string(p->args, "section"), control_codec_string(p->args, "key"));
    if (!field || !automation_manager_setting_changeable(s->deps.automation, field)) {
        f->code = "not_allowed";
        str_copy(f->why, sizeof(f->why), "That setting cannot be changed from outside tawk");
        return NULL;
    }
    Settings updated = *control_settings(s);
    setting_set_from_text(&updated, field, control_codec_string(p->args, "value"));
    if (settings_manager_apply(s->deps.settings, &updated) != 0) {
        str_copy(f->why, sizeof(f->why), "The settings could not be saved");
        return NULL;
    }
    char value[600];
    setting_to_text(control_settings(s), field, value, sizeof(value));
    cJSON *r = cJSON_CreateObject();
    cJSON_AddStringToObject(r, "value", value);
    if (field->requires_restart) cJSON_AddBoolToObject(r, "restart_needed", 1);
    return r;
}

void control_op_set_setting(ControlServer *s, ControlSession *session, const ControlRequest *req) {
    const char *section = control_required(s, session, req, "section");
    if (!section) return;
    const char *key = control_required(s, session, req, "key");
    if (!key) return;
    const char *value = control_codec_string(req->args, "value");
    if (!value) { control_fail(s, session->conn, req->id, "bad_request", "\"value\" is required, as text"); return; }
    const SettingField *field = field_named(section, key);
    if (!field) { control_fail(s, session->conn, req->id, "not_found", "No such setting (see get_settings)"); return; }
    if (!automation_manager_setting_changeable(s->deps.automation, field)) {
        control_fail(s, session->conn, req->id, "not_allowed", "That setting can only be changed in tawk itself");
        return;
    }
    Settings probe = *control_settings(s);
    setting_set_from_text(&probe, field, value);
    char before[600], after[600];
    setting_to_text(control_settings(s), field, before, sizeof(before));
    setting_to_text(&probe, field, after, sizeof(after));
    ControlPending p;
    control_pending_init(&p, req->id, "set_setting", WRITE_KIND_MANAGE, do_set);
    cJSON_AddStringToObject(p.args, "section", section);
    cJSON_AddStringToObject(p.args, "key", key);
    cJSON_AddStringToObject(p.args, "value", value);
    snprintf(p.action, sizeof(p.action), "change %.40s from %.30s to %.30s", field->label, before, after);
    control_write(s, session, &p);
}

void control_op_list_themes(ControlServer *s, ControlSession *session, const ControlRequest *req) {
    IThemeRepository *themes = settings_manager_themes(s->deps.settings);
    cJSON *r = cJSON_CreateObject();
    cJSON *list = cJSON_AddArrayToObject(r, "themes");
    for (int i = 0; i < themes->count(themes); i++) {
        const Theme *t = themes->at(themes, i);
        cJSON *o = cJSON_CreateObject();
        cJSON_AddStringToObject(o, "id", t->id);
        cJSON_AddStringToObject(o, "name", t->name);
        cJSON_AddItemToArray(list, o);
    }
    cJSON_AddStringToObject(r, "current", control_settings(s)->theme);
    control_reply(s, session->conn, control_codec_ok(req->id, r));
}

/* An agent says what it is working on, so you can tell its sessions apart in the Agents list.
 * It changes nothing but that line, so it is a read. "" takes the line away. */
void control_op_describe(ControlServer *s, ControlSession *session, const ControlRequest *req) {
    const char *text = control_codec_string(req->args, "text");
    str_copy(session->doing, sizeof(session->doing), text ? text : "");
    str_strip_controls(session->doing);
    s->changed = 1;
    control_reply(s, session->conn, control_codec_ok(req->id, NULL));
}

void control_op_app_status(ControlServer *s, ControlSession *session, const ControlRequest *req) {
    ConnectionHealth h;
    messaging_manager_health(s->deps.messaging, &h);
    cJSON *r = cJSON_CreateObject();
    cJSON_AddStringToObject(r, "tawk", APP_VERSION);
    cJSON_AddStringToObject(r, "backend", s->deps.backend_name ? s->deps.backend_name : "");
    cJSON_AddBoolToObject(r, "connected", control_connected(s));
    cJSON_AddStringToObject(r, "state", h.available ? "connected" : h.title);
    cJSON_AddStringToObject(r, "detail", h.detail);
    const IncomingCall *call = s->deps.calls ? call_manager_ringing(s->deps.calls) : NULL;
    if (call) {
        char name[128];
        messaging_manager_display_name(s->deps.messaging, call->from, name, sizeof(name));
        cJSON *c = cJSON_AddObjectToObject(r, "ringing");
        cJSON_AddStringToObject(c, "from", call->from);
        cJSON_AddStringToObject(c, "name", name);
    }
    control_reply(s, session->conn, control_codec_ok(req->id, r));
}

static cJSON *do_reconnect(ControlServer *s, const ControlPending *p, ControlFailure *f) {
    (void)p; (void)f;
    messaging_manager_retry_now(s->deps.messaging);
    return cJSON_CreateObject();
}

void control_op_reconnect(ControlServer *s, ControlSession *session, const ControlRequest *req) {
    ControlPending p;
    control_pending_init(&p, req->id, "reconnect", WRITE_KIND_MANAGE, do_reconnect);
    str_copy(p.action, sizeof(p.action), "reconnect to WhatsApp now");
    control_write(s, session, &p);
}

static cJSON *do_decline(ControlServer *s, const ControlPending *p, ControlFailure *f) {
    (void)p;
    if (!s->deps.calls || call_manager_decline(s->deps.calls) != 0) {
        str_copy(f->why, sizeof(f->why), "No call is ringing");
        return NULL;
    }
    return cJSON_CreateObject();
}

void control_op_decline_call(ControlServer *s, ControlSession *session, const ControlRequest *req) {
    const IncomingCall *call = s->deps.calls ? call_manager_ringing(s->deps.calls) : NULL;
    if (!call) { control_fail(s, session->conn, req->id, "not_found", "No call is ringing"); return; }
    ControlPending p;
    control_pending_init(&p, req->id, "decline_call", WRITE_KIND_MANAGE, do_decline);
    str_copy(p.chat_jid, sizeof(p.chat_jid), call->from);
    str_copy(p.action, sizeof(p.action), "decline the call ringing now");
    p.needs_connection = 1;
    control_write(s, session, &p);
}
