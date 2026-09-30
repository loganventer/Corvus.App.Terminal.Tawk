#ifndef APP_MANAGERS_PROFILE_MANAGER_H
#define APP_MANAGERS_PROFILE_MANAGER_H

#include "contracts/i_event_observer.h"
#include "core/contact_profile.h"
#include "managers/profile_manager_deps.h"

/* Contact and group details, profile pictures and the block list: asks the
 * backend for what is missing or stale (a few requests at a time), keeps
 * the answers, and hands them to the screens. */
typedef struct ProfileManager ProfileManager;

ProfileManager *profile_manager_create(const ProfileManagerDeps *deps);
void            profile_manager_destroy(ProfileManager *mgr);
/* The observer to hand to the messaging manager, so profile events reach here. */
IEventObserver *profile_manager_observer(ProfileManager *mgr);
/* Sends queued requests; call once per loop. */
void            profile_manager_tick(ProfileManager *mgr);

/* The preview picture's path when there is one, else NULL. Asks for it
 * (and the details) when it has not been fetched yet or is a day old. */
const char     *profile_manager_picture(ProfileManager *mgr, const char *jid);
/* The full-size picture's path, or NULL while it is being fetched. */
const char     *profile_manager_full_picture(ProfileManager *mgr, const char *jid);
/* Details, fetched again when older than an hour (`refresh` forces it).
 * Fills `out` (caller disposes); returns -1 when nothing is known yet. */
int             profile_manager_details(ProfileManager *mgr, const char *jid, int refresh, ContactProfile *out);
int             profile_manager_is_blocked(ProfileManager *mgr, const char *jid);
/* A short line about the contact for a title bar: the about text, the
 * business category, or "N members" for a group. Valid until the next call. */
const char     *profile_manager_summary(ProfileManager *mgr, const char *jid);
int             profile_manager_set_blocked(ProfileManager *mgr, const char *jid, int blocked);
/* Non-zero while requests are waiting to be answered (a spinner may show). */
int             profile_manager_busy(ProfileManager *mgr);

#endif
