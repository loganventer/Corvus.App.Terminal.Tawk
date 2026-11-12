#ifndef APP_MANAGERS_PRIVACY_MANAGER_H
#define APP_MANAGERS_PRIVACY_MANAGER_H

#include <stdint.h>

#include "core/biometric_result.h"
#include "core/lock_password_problem.h"
#include "core/passphrase.h"
#include "core/privacy_check.h"
#include "core/unlock_purpose.h"
#include "managers/privacy_manager_deps.h"

/* Private chats' secrets for the whole computer, not one account: setting
 * and checking the lock password, slowing wrong tries down, and asking
 * Touch ID. Every passphrase handed to it is wiped before it returns. */
typedef struct PrivacyManager PrivacyManager;

PrivacyManager     *privacy_manager_create(const PrivacyManagerDeps *deps);
void                privacy_manager_destroy(PrivacyManager *mgr);

int                 privacy_manager_has_password(PrivacyManager *mgr);
/* Sets a new lock password, typed twice, with a fresh salt. */
LockPasswordProblem privacy_manager_set_password(PrivacyManager *mgr, Passphrase *password, Passphrase *confirm);
/* Checks a typed password. After three wrong tries each one waits longer;
 * *wait_ms says how long until the next is looked at. The right one forgets
 * the count. The count lives only as long as tawk runs. */
PrivacyCheck        privacy_manager_check_password(PrivacyManager *mgr, Passphrase *password, int64_t now_ms, int64_t *wait_ms);

/* Touch ID is switched on in the settings and the Mac can use it here. */
int                 privacy_manager_biometrics_available(PrivacyManager *mgr);
/* Puts the Touch ID sheet up for `purpose`; -1 when it cannot. */
int                 privacy_manager_begin_biometric(PrivacyManager *mgr, UnlockPurpose purpose);
BiometricResult     privacy_manager_poll_biometric(PrivacyManager *mgr);
void                privacy_manager_cancel_biometric(PrivacyManager *mgr);

#endif
