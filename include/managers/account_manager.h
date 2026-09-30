#ifndef APP_MANAGERS_ACCOUNT_MANAGER_H
#define APP_MANAGERS_ACCOUNT_MANAGER_H

#include "contracts/i_event_observer.h"
#include "core/profile_edit_result.h"
#include "core/profile_field.h"
#include "managers/account_manager_deps.h"

/* Your own profile: which account is linked, and changing its name, about
 * text and photo. Viewing your details goes through the profile manager
 * like any other contact's. */
typedef struct AccountManager AccountManager;

AccountManager *account_manager_create(const AccountManagerDeps *deps);
void            account_manager_destroy(AccountManager *mgr);
/* The observer to hand to the messaging manager (connected and profile_updated events). */
IEventObserver *account_manager_observer(AccountManager *mgr);
/* Gives up on requests the backend has not answered; call once per loop. */
void            account_manager_tick(AccountManager *mgr);

/* Your JID and name as linked, "" before the first connect. */
const char     *account_manager_user_jid(AccountManager *mgr);
const char     *account_manager_user_name(AccountManager *mgr);

/* Each returns 0 when the change was sent, or -1 with the reason in
 * account_manager_error (too long, empty, not connected, one already running). */
int             account_manager_set_name(AccountManager *mgr, const char *name);
int             account_manager_set_about(AccountManager *mgr, const char *text);
int             account_manager_set_picture(AccountManager *mgr, const char *path);
int             account_manager_remove_picture(AccountManager *mgr);
const char     *account_manager_error(AccountManager *mgr);
/* The most characters `field` may hold (WhatsApp's limit). */
int             account_manager_max_chars(AccountManager *mgr, ProfileField field);
/* True while a change to `field` waits for the backend. */
int             account_manager_busy(AccountManager *mgr, ProfileField field);
/* Hands over one finished change; returns 0 when there was one. */
int             account_manager_take_result(AccountManager *mgr, ProfileEditResult *out);

#endif
