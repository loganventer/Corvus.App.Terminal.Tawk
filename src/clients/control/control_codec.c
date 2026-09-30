#include "clients/control/control_codec.h"
#include "utilities/str_util.h"

#include <stdlib.h>
#include <string.h>

static const char *const STATUS_NAMES[] = { "pending", "sent", "delivered", "read", "failed" };

int control_codec_parse(const char *line, ControlRequest *out) {
    memset(out, 0, sizeof(*out));
    cJSON *root = cJSON_Parse(line);
    if (!cJSON_IsObject(root)) { cJSON_Delete(root); return -1; }
    out->root = root;
    const cJSON *id = cJSON_GetObjectItemCaseSensitive(root, "id");
    const cJSON *op = cJSON_GetObjectItemCaseSensitive(root, "op");
    if (cJSON_IsString(id) && strlen(id->valuestring) < sizeof(out->id)) str_copy(out->id, sizeof(out->id), id->valuestring);
    if (!out->id[0] || !cJSON_IsString(op) || strlen(op->valuestring) >= sizeof(out->op)) return -1;
    str_copy(out->op, sizeof(out->op), op->valuestring);
    cJSON *args = cJSON_GetObjectItemCaseSensitive(root, "args");
    if (!args) args = cJSON_AddObjectToObject(root, "args");
    if (!cJSON_IsObject(args)) return -1;
    out->args = args;
    return 0;
}

cJSON *control_codec_chat(const Chat *c) {
    cJSON *o = cJSON_CreateObject();
    cJSON_AddStringToObject(o, "jid", c->jid);
    cJSON_AddStringToObject(o, "name", c->name);
    cJSON_AddBoolToObject(o, "is_group", c->is_group);
    cJSON_AddNumberToObject(o, "unread", c->unread > 0 ? c->unread : 0);
    cJSON_AddBoolToObject(o, "unread_mention", c->unread_mention);
    cJSON_AddBoolToObject(o, "muted", c->is_muted);
    cJSON_AddBoolToObject(o, "pinned", c->is_pinned);
    cJSON_AddBoolToObject(o, "archived", c->is_archived > 0);
    cJSON_AddNumberToObject(o, "last_ts", (double)c->last_ts);
    cJSON_AddStringToObject(o, "preview", c->preview);
    return o;
}

cJSON *control_codec_message(const Message *m, const char *sender_name) {
    cJSON *o = cJSON_CreateObject();
    cJSON_AddStringToObject(o, "id", m->id);
    cJSON_AddStringToObject(o, "chat", m->chat_jid);
    cJSON_AddStringToObject(o, "sender", m->sender_jid);
    cJSON_AddStringToObject(o, "sender_name", sender_name ? sender_name : "");
    cJSON_AddBoolToObject(o, "from_me", m->from_me);
    cJSON_AddNumberToObject(o, "ts", (double)m->timestamp);
    cJSON_AddStringToObject(o, "type", message_type_name(m->type));
    if (m->text && m->text[0] && !m->deleted) cJSON_AddStringToObject(o, "text", m->text);
    if (m->from_me && m->status >= 0 && m->status <= MESSAGE_STATUS_FAILED) cJSON_AddStringToObject(o, "status", STATUS_NAMES[m->status]);
    cJSON_AddBoolToObject(o, "edited", m->edited);
    cJSON_AddBoolToObject(o, "deleted", m->deleted);
    cJSON_AddBoolToObject(o, "forwarded", m->forwarded);
    cJSON_AddBoolToObject(o, "mentions_me", m->mentions_me);
    if (m->quoted_id[0]) {
        cJSON *r = cJSON_AddObjectToObject(o, "reply_to");
        cJSON_AddStringToObject(r, "id", m->quoted_id);
        cJSON_AddStringToObject(r, "sender", m->quoted_sender);
        cJSON_AddStringToObject(r, "text", m->quoted_text ? m->quoted_text : "");
        cJSON_AddBoolToObject(r, "status", m->quoted_status);
    }
    if (m->reactions[0]) cJSON_AddStringToObject(o, "reactions", m->reactions);
    if (m->link && m->link->url[0]) {
        cJSON *l = cJSON_AddObjectToObject(o, "link");
        cJSON_AddStringToObject(l, "url", m->link->url);
        cJSON_AddStringToObject(l, "title", m->link->title);
        cJSON_AddStringToObject(l, "description", m->link->description);
    }
    return o;
}

static char *print_and_free(cJSON *o) {
    char *line = cJSON_PrintUnformatted(o);
    cJSON_Delete(o);
    return line;
}

char *control_codec_ok(const char *id, cJSON *result) {
    cJSON *o = cJSON_CreateObject();
    cJSON_AddStringToObject(o, "id", id ? id : "");
    cJSON_AddBoolToObject(o, "ok", 1);
    cJSON_AddItemToObject(o, "result", result ? result : cJSON_CreateObject());
    return print_and_free(o);
}

char *control_codec_error(const char *id, const char *code, const char *message, cJSON *extra) {
    cJSON *o = cJSON_CreateObject();
    if (id && id[0]) cJSON_AddStringToObject(o, "id", id); else cJSON_AddNullToObject(o, "id");
    cJSON_AddBoolToObject(o, "ok", 0);
    cJSON *e = extra ? extra : cJSON_CreateObject();
    cJSON_AddStringToObject(e, "code", code);
    cJSON_AddStringToObject(e, "message", message ? message : "");
    cJSON_AddItemToObject(o, "error", e);
    return print_and_free(o);
}

char *control_codec_event(const char *evt, cJSON *fields) {
    cJSON *o = cJSON_CreateObject();
    cJSON_AddStringToObject(o, "evt", evt);          /* first, for readers skimming the stream */
    while (fields && fields->child) {
        cJSON *item = cJSON_DetachItemViaPointer(fields, fields->child);
        cJSON_AddItemToObject(o, item->string, item);
    }
    cJSON_Delete(fields);
    return print_and_free(o);
}

const char *control_codec_string(const cJSON *args, const char *name) {
    const cJSON *v = cJSON_GetObjectItemCaseSensitive(args, name);
    return cJSON_IsString(v) ? v->valuestring : NULL;
}

int64_t control_codec_int(const cJSON *args, const char *name, int64_t fallback, int64_t min, int64_t max) {
    const cJSON *v = cJSON_GetObjectItemCaseSensitive(args, name);
    if (!cJSON_IsNumber(v)) return fallback;
    double d = v->valuedouble;
    if (d < (double)min) return min;
    if (d > (double)max) return max;
    return (int64_t)d;
}

int control_codec_bool(const cJSON *args, const char *name, int fallback) {
    const cJSON *v = cJSON_GetObjectItemCaseSensitive(args, name);
    return cJSON_IsBool(v) ? cJSON_IsTrue(v) : fallback;
}
