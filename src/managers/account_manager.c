#include "managers/account_manager.h"
#include "engines/message_id_generator.h"
#include "engines/profile_field_validator.h"
#include "utilities/clock_util.h"
#include "utilities/outgoing_media.h"
#include "utilities/path_util.h"
#include "utilities/str_util.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define ANSWER_TIMEOUT_MS 30000
#define MAX_RESULTS       8

struct AccountManager {
    AccountManagerDeps deps;
    IEventObserver     observer;
    char               user_jid[128];
    char               user_name[128];
    char               pending_name[128];              /* the name being set, shown once it is saved */
    int64_t            deadline_ms[PROFILE_FIELD_COUNT]; /* 0 when nothing is waiting */
    ProfileEditResult  results[MAX_RESULTS];
    int                result_count;
    char               error[256];
};

static void push_result(AccountManager *m, ProfileField field, int ok, const char *detail) {
    if (m->result_count >= MAX_RESULTS) {
        memmove(&m->results[0], &m->results[1], sizeof(m->results[0]) * (MAX_RESULTS - 1));
        m->result_count--;
    }
    ProfileEditResult *r = &m->results[m->result_count++];
    r->field = field;
    r->ok = ok;
    str_copy(r->detail, sizeof(r->detail), ok ? "" : (detail && *detail ? detail : "WhatsApp refused the change."));
}

static int refuse(AccountManager *m, const char *reason) {
    str_copy(m->error, sizeof(m->error), reason);
    return -1;
}

/* Common checks before a change goes to the backend. */
static int ready(AccountManager *m, ProfileField field) {
    m->error[0] = '\0';
    if (!m->deps.editor) return refuse(m, "This backend cannot change your profile.");
    if (m->deadline_ms[field]) return refuse(m, "A change to this is still being saved.");
    return 0;
}

static int sent(AccountManager *m, ProfileField field, int rc) {
    if (rc != 0) return refuse(m, "The WhatsApp bridge did not take the change.");
    m->deadline_ms[field] = clock_now_ms() + ANSWER_TIMEOUT_MS;
    return 0;
}

static int set_text(AccountManager *m, ProfileField field, const char *text) {
    if (ready(m, field) != 0) return -1;
    if (profile_field_validate(field, text, m->error, sizeof(m->error)) != 0) return -1;
    IProfileEditor *ed = m->deps.editor;
    if (field == PROFILE_FIELD_NAME) str_copy(m->pending_name, sizeof(m->pending_name), text);
    return sent(m, field, field == PROFILE_FIELD_NAME ? ed->set_name(ed, text) : ed->set_about(ed, text));
}

static int on_event(IEventObserver *self, const Event *e) {
    AccountManager *m = self->ctx;
    switch (e->type) {
        case EVENT_AUTH_CONNECTED:
            str_copy(m->user_jid, sizeof(m->user_jid), e->jid);
            if (e->name[0] || !m->user_name[0]) str_copy(m->user_name, sizeof(m->user_name), e->name);
            return 1;
        case EVENT_PROFILE_UPDATED: {
            ProfileField field = profile_field_parse(e->reason);
            if (field == PROFILE_FIELD_COUNT) return 0;
            m->deadline_ms[field] = 0;
            if (e->ok && field == PROFILE_FIELD_NAME) {
                str_copy(m->user_name, sizeof(m->user_name), e->name[0] ? e->name : m->pending_name);
            }
            push_result(m, field, e->ok, e->detail);
            return 1;
        }
        default:
            return 0;
    }
}

static void observer_destroy(IEventObserver *self) { (void)self; }   /* owned by the manager */

AccountManager *account_manager_create(const AccountManagerDeps *deps) {
    AccountManager *m = calloc(1, sizeof(*m));
    if (!m) return NULL;
    m->deps = *deps;
    m->observer.ctx = m;
    m->observer.on_event = on_event;
    m->observer.destroy = observer_destroy;
    return m;
}

void account_manager_destroy(AccountManager *m) { free(m); }

IEventObserver *account_manager_observer(AccountManager *m) { return &m->observer; }

void account_manager_tick(AccountManager *m) {
    int64_t now = clock_now_ms();
    for (int f = 0; f < PROFILE_FIELD_COUNT; f++) {
        if (!m->deadline_ms[f] || now < m->deadline_ms[f]) continue;
        m->deadline_ms[f] = 0;
        push_result(m, (ProfileField)f, 0, "WhatsApp did not answer.");
    }
}

const char *account_manager_user_jid(AccountManager *m) { return m->user_jid; }
const char *account_manager_user_name(AccountManager *m) { return m->user_name; }

int account_manager_set_name(AccountManager *m, const char *name) { return set_text(m, PROFILE_FIELD_NAME, name); }
int account_manager_set_about(AccountManager *m, const char *text) { return set_text(m, PROFILE_FIELD_ABOUT, text); }

int account_manager_set_picture(AccountManager *m, const char *path) {
    if (ready(m, PROFILE_FIELD_PICTURE) != 0) return -1;
    if (!path || !path_is_regular_file(path)) return refuse(m, "That file cannot be read.");
    char id[64], copy[600];
    if (message_id_generate(id, sizeof(id)) != 0 ||
        outgoing_media_copy(m->deps.media_dir, path, id, copy, sizeof(copy)) != 0) {
        return refuse(m, "The picture could not be copied (files over 100 MB are refused).");
    }
    return sent(m, PROFILE_FIELD_PICTURE, m->deps.editor->set_picture(m->deps.editor, copy));
}

int account_manager_remove_picture(AccountManager *m) {
    if (ready(m, PROFILE_FIELD_PICTURE) != 0) return -1;
    return sent(m, PROFILE_FIELD_PICTURE, m->deps.editor->remove_picture(m->deps.editor));
}

const char *account_manager_error(AccountManager *m) { return m->error; }

int account_manager_max_chars(AccountManager *m, ProfileField field) {
    (void)m;
    return profile_field_max_chars(field);
}

int account_manager_busy(AccountManager *m, ProfileField field) {
    return field >= 0 && field < PROFILE_FIELD_COUNT && m->deadline_ms[field] != 0;
}

int account_manager_take_result(AccountManager *m, ProfileEditResult *out) {
    if (m->result_count == 0) return -1;
    *out = m->results[0];
    memmove(&m->results[0], &m->results[1], sizeof(m->results[0]) * (size_t)(m->result_count - 1));
    m->result_count--;
    return 0;
}
