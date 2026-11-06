/* Notification sounds over the control socket: the sounds there are, and the
 * one a chat plays. An agent names a sound from the catalog by its id, never a
 * file: only you can point a chat at a file of your own, in tawk. */
#include "control_server_state.h"

#include "engines/sound_name.h"
#include "utilities/str_util.h"

#include <stdio.h>
#include <string.h>

static const char NO_SOUNDS[] = "This tawk has no sound folders";

/* list_sounds: {"sounds":[{"id"}],"current":"<app sound id>"}; a read. */
void control_op_list_sounds(ControlServer *s, ControlSession *session, const ControlRequest *req) {
    ISoundCatalog *sounds = s->deps.sounds;
    if (!sounds) { control_fail(s, session->conn, req->id, "not_found", NO_SOUNDS); return; }
    sounds->reload(sounds);
    cJSON *r = cJSON_CreateObject();
    cJSON *list = cJSON_AddArrayToObject(r, "sounds");
    for (int i = 0; i < sounds->count(sounds); i++) {
        cJSON *o = cJSON_CreateObject();
        cJSON_AddStringToObject(o, "id", sounds->at(sounds, i)->id);
        cJSON_AddItemToArray(list, o);
    }
    char current[64] = "";
    const char *file = control_settings(s)->sound_file;
    if (file[0]) sound_name_label(file, current, sizeof(current));
    cJSON_AddStringToObject(r, "current", current);
    control_reply(s, session->conn, control_codec_ok(req->id, r));
}

/* ---- set_chat_tone ---- */

static cJSON *do_tone(ControlServer *s, const ControlPending *p, ControlFailure *f) {
    (void)f;
    messaging_manager_set_tone(s->deps.messaging, p->chat_jid, control_codec_string(p->args, "stored"));
    return cJSON_CreateObject();
}

/* set_chat_tone: "tone" is a sound id from list_sounds, "none" for silence,
 * or "" for the app's sound. */
void control_op_set_chat_tone(ControlServer *s, ControlSession *session, const ControlRequest *req) {
    const char *tone = control_codec_string(req->args, "tone");
    if (!tone) {
        control_fail(s, session->conn, req->id, "bad_request", "\"tone\" is required (a sound id, \"none\", or \"\" for the app sound)");
        return;
    }
    const char *stored = tone;
    if (tone[0] && strcmp(tone, "none") != 0) {
        ISoundCatalog *sounds = s->deps.sounds;
        if (!sounds) { control_fail(s, session->conn, req->id, "not_found", NO_SOUNDS); return; }
        sounds->reload(sounds);
        int at = sounds->index_of_id(sounds, tone);
        if (at < 0) { control_fail(s, session->conn, req->id, "not_found", "No such sound (see list_sounds)"); return; }
        stored = sounds->at(sounds, at)->path;
    }
    int n = 0;
    const Chat *all = messaging_manager_chats(s->deps.messaging, &n);
    int at = control_resolve_chat(s, session, req, "chat", all, n);
    if (at < 0) return;
    ControlPending p;
    control_pending_init(&p, req->id, "set_chat_tone", WRITE_KIND_MANAGE, do_tone);
    str_copy(p.chat_jid, sizeof(p.chat_jid), all[at].jid);
    cJSON_AddStringToObject(p.args, "tone", tone);
    cJSON_AddStringToObject(p.args, "stored", stored);
    if (!tone[0]) str_copy(p.action, sizeof(p.action), "give it the app's sound again");
    else if (!strcmp(tone, "none")) str_copy(p.action, sizeof(p.action), "make it silent");
    else snprintf(p.action, sizeof(p.action), "give it the %.60s tone", tone);
    control_write(s, session, &p);
}
