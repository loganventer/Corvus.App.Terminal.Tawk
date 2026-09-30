#include "managers/scheduling_manager.h"
#include "engines/message_id_generator.h"
#include "engines/schedule_time_parser.h"
#include "utilities/str_util.h"

#include <stdlib.h>
#include <string.h>

#define MAX_TEXT_BYTES 65536

struct SchedulingManager {
    SchedulingManagerDeps deps;
    IEventObserver        observer;
    char                  error[160];
    int                   changed;
};

static int refuse(SchedulingManager *m, const char *why) {
    str_copy(m->error, sizeof(m->error), why);
    return -1;
}

static int changed(SchedulingManager *m, int rc) {
    if (rc == 0) m->changed = 1;
    return rc;
}

static int observe(IEventObserver *self, const Event *e) {
    SchedulingManager *m = self->ctx;
    if (e->type != EVENT_JID_ALIAS || !e->lid[0] || !e->jid[0]) return 0;
    return changed(m, m->deps.store->reassign_jid(m->deps.store, e->lid, e->jid)) == 0;
}

static void observer_destroy(IEventObserver *self) { (void)self; }   /* owned by the manager */

SchedulingManager *scheduling_manager_create(const SchedulingManagerDeps *deps) {
    SchedulingManager *m = calloc(1, sizeof(*m));
    if (!m) return NULL;
    m->deps = *deps;
    m->observer = (IEventObserver){ m, observe, observer_destroy };
    return m;
}

void scheduling_manager_destroy(SchedulingManager *m) { free(m); }
IEventObserver *scheduling_manager_observer(SchedulingManager *m) { return &m->observer; }
const char *scheduling_manager_error(SchedulingManager *m) { return m->error; }

int scheduling_manager_schedule(SchedulingManager *m, const char *jid, const char *text, const char *mentions,
                                int64_t due, int64_t now, char *id_out, size_t id_size) {
    m->error[0] = '\0';
    if (!jid || !jid[0]) return refuse(m, "Open a chat first.");
    if (!text || !text[0]) return refuse(m, "Write the message to send.");
    if (strlen(text) > MAX_TEXT_BYTES) return refuse(m, "That message is too long.");
    if (due <= now) return refuse(m, "Choose a time in the future.");
    ScheduledMessage s;
    scheduled_message_init(&s);
    if (message_id_generate(s.id, sizeof(s.id)) != 0) return refuse(m, "Could not make an id for it.");
    str_copy(s.chat_jid, sizeof(s.chat_jid), jid);
    s.text = (char *)text;                              /* borrowed for the insert */
    s.mentions = (char *)mentions;
    s.due_at = due;
    s.created_at = now;
    s.state = SCHEDULED_WAITING;
    if (m->deps.store->add(m->deps.store, &s) != 0) return refuse(m, "It could not be saved.");
    if (id_out) str_copy(id_out, id_size, s.id);
    m->changed = 1;
    return 0;
}

int scheduling_manager_reschedule(SchedulingManager *m, const char *id, int64_t due, int64_t now) {
    m->error[0] = '\0';
    if (due <= now) return refuse(m, "Choose a time in the future.");
    if (m->deps.store->set_due(m->deps.store, id, due) != 0) return refuse(m, "That message is no longer waiting.");
    m->changed = 1;
    return 0;
}

#define WHEN_HELP "Start with when to send it: 18:00, +30m, tomorrow 9:00 or fri 17:30."

int scheduling_manager_schedule_line(SchedulingManager *m, const char *jid, const char *line, int64_t now,
                                     int64_t *due_out, char *id_out, size_t id_size) {
    int64_t due = 0;
    const char *rest = NULL;
    m->error[0] = '\0';
    if (schedule_time_parse(line, now, &due, &rest) != 0) return refuse(m, WHEN_HELP);
    if (due_out) *due_out = due;
    return scheduling_manager_schedule(m, jid, rest, NULL, due, now, id_out, id_size);
}

int scheduling_manager_reschedule_text(SchedulingManager *m, const char *id, const char *when, int64_t now, int64_t *due_out) {
    int64_t due = 0;
    const char *rest = NULL;
    m->error[0] = '\0';
    if (schedule_time_parse(when, now, &due, &rest) != 0 || (rest && *rest)) {
        return refuse(m, "Type a time such as 18:00, +30m, tomorrow 9:00 or fri 17:30.");
    }
    if (due_out) *due_out = due;
    return scheduling_manager_reschedule(m, id, due, now);
}

int scheduling_manager_parse_when(SchedulingManager *m, const char *when, int64_t now, int64_t *due_out) {
    int64_t due = 0;
    const char *rest = NULL;
    m->error[0] = '\0';
    if (!when || schedule_time_parse(when, now, &due, &rest) != 0 || (rest && *rest) || due <= now) {
        return refuse(m, "Give a time such as 18:00, +30m, tomorrow 9:00 or fri 17:30.");
    }
    *due_out = due;
    return 0;
}

int scheduling_manager_send_now(SchedulingManager *m, const char *id, int64_t now) {
    return changed(m, m->deps.store->set_due(m->deps.store, id, now));
}

int scheduling_manager_cancel(SchedulingManager *m, const char *id) {
    return changed(m, m->deps.store->remove(m->deps.store, id));
}

int scheduling_manager_list(SchedulingManager *m, const char *jid, ScheduledMessage **out, int *count) {
    return m->deps.store->list_waiting(m->deps.store, jid, out, count);
}

int scheduling_manager_take_due(SchedulingManager *m, int64_t now, ScheduledMessage **out, int *count) {
    return m->deps.store->due(m->deps.store, now, out, count);
}

/* A sent message lives on as an ordinary message, so its schedule goes. */
void scheduling_manager_mark_sent(SchedulingManager *m, const char *id) {
    changed(m, m->deps.store->remove(m->deps.store, id));
}

void scheduling_manager_mark_failed(SchedulingManager *m, const char *id) {
    changed(m, m->deps.store->set_state(m->deps.store, id, SCHEDULED_FAILED));
}

int scheduling_manager_take_changed(SchedulingManager *m) {
    int c = m->changed;
    m->changed = 0;
    return c;
}
