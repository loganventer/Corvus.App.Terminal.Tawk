#ifndef APP_MANAGERS_SCHEDULING_MANAGER_H
#define APP_MANAGERS_SCHEDULING_MANAGER_H

#include <stddef.h>
#include <stdint.h>

#include "contracts/i_event_observer.h"
#include "core/scheduled_message.h"
#include "managers/scheduling_manager_deps.h"

/* Messages to send later: keeps them on this computer until they are due
 * and says which are. It sends nothing itself; whoever runs the loop hands
 * due messages to the messaging manager and reports back with mark_sent or
 * mark_failed, so the two managers stay independent. */
typedef struct SchedulingManager SchedulingManager;

SchedulingManager *scheduling_manager_create(const SchedulingManagerDeps *deps);
void               scheduling_manager_destroy(SchedulingManager *mgr);
/* The observer to hand to the messaging manager (chats that turn out to be another JID). */
IEventObserver    *scheduling_manager_observer(SchedulingManager *mgr);

/* Keeps `text` for `jid` until `due` (epoch seconds, in the future). Writes
 * its id to id_out. Returns 0, or -1 with the reason in scheduling_manager_error. */
int  scheduling_manager_schedule(SchedulingManager *mgr, const char *jid, const char *text, const char *mentions,
                                 int64_t due, int64_t now, char *id_out, size_t id_size);
int  scheduling_manager_reschedule(SchedulingManager *mgr, const char *id, int64_t due, int64_t now);
/* The same, reading the time from the start of `line` ("18:00 see you",
 * "+30m", "tomorrow 9:00 ..."); schedule_line sends the rest of the line.
 * *due_out gets the time chosen. */
int  scheduling_manager_schedule_line(SchedulingManager *mgr, const char *jid, const char *line, int64_t now,
                                      int64_t *due_out, char *id_out, size_t id_size);
int  scheduling_manager_reschedule_text(SchedulingManager *mgr, const char *id, const char *when, int64_t now, int64_t *due_out);
/* Reads a time on its own ("18:00", "+30m", "fri 17:30") without keeping anything. */
int  scheduling_manager_parse_when(SchedulingManager *mgr, const char *when, int64_t now, int64_t *due_out);
/* Makes a waiting message due now; it goes on the next tick while connected. */
int  scheduling_manager_send_now(SchedulingManager *mgr, const char *id, int64_t now);
int  scheduling_manager_cancel(SchedulingManager *mgr, const char *id);
const char *scheduling_manager_error(SchedulingManager *mgr);

/* Waiting messages, soonest first, for one chat or all (NULL); free with scheduled_message_array_free. */
int  scheduling_manager_list(SchedulingManager *mgr, const char *jid, ScheduledMessage **out, int *count);
/* Waiting messages due by `now`; send each, then mark it. */
int  scheduling_manager_take_due(SchedulingManager *mgr, int64_t now, ScheduledMessage **out, int *count);
void scheduling_manager_mark_sent(SchedulingManager *mgr, const char *id);
void scheduling_manager_mark_failed(SchedulingManager *mgr, const char *id);
/* True once after anything scheduled changed. */
int  scheduling_manager_take_changed(SchedulingManager *mgr);

#endif
