#include "managers/privacy_manager.h"
#include "engines/idle_lock_policy.h"
#include "engines/lock_password_hasher.h"
#include "engines/lock_password_rule.h"
#include "utilities/secure_random.h"

#include <stdlib.h>

struct PrivacyManager {
    PrivacyManagerDeps deps;
    int                failed;        /* wrong tries since the last right one */
    int64_t            retry_at_ms;   /* no try is looked at before this */
};

PrivacyManager *privacy_manager_create(const PrivacyManagerDeps *deps) {
    if (!deps || !deps->secrets || !deps->biometrics) return NULL;
    PrivacyManager *m = calloc(1, sizeof(*m));
    if (!m) return NULL;
    m->deps = *deps;
    return m;
}

void privacy_manager_destroy(PrivacyManager *m) {
    if (!m) return;
    m->deps.biometrics->cancel(m->deps.biometrics);
    free(m);
}

int privacy_manager_has_password(PrivacyManager *m) {
    LockVerifier v;
    int found = m->deps.secrets->load(m->deps.secrets, &v) == 1;
    lock_verifier_wipe(&v);
    return found;
}

LockPasswordProblem privacy_manager_set_password(PrivacyManager *m, Passphrase *password, Passphrase *confirm) {
    LockPasswordProblem problem = lock_password_rule_check(password, confirm);
    if (problem == LOCK_PASSWORD_OK) {
        uint8_t salt[LOCK_VERIFIER_SALT_BYTES];
        LockVerifier v;
        if (secure_random_bytes(salt, sizeof(salt)) == 0) {
            lock_password_hasher_derive(password, salt, &v);
            if (m->deps.secrets->save(m->deps.secrets, &v) == 0) {
                m->failed = 0;
                m->retry_at_ms = 0;
            } else {
                problem = LOCK_PASSWORD_NOT_SAVED;
            }
        } else {
            problem = LOCK_PASSWORD_NOT_SAVED;
        }
        lock_verifier_wipe(&v);
    }
    passphrase_wipe(password);
    if (confirm) passphrase_wipe(confirm);
    return problem;
}

PrivacyCheck privacy_manager_check_password(PrivacyManager *m, Passphrase *password, int64_t now_ms, int64_t *wait_ms) {
    PrivacyCheck result;
    LockVerifier v;
    if (wait_ms) *wait_ms = 0;
    if (now_ms < m->retry_at_ms) {
        if (wait_ms) *wait_ms = m->retry_at_ms - now_ms;
        result = PRIVACY_CHECK_WAIT;
    } else if (m->deps.secrets->load(m->deps.secrets, &v) != 1) {
        result = PRIVACY_CHECK_NO_PASSWORD;
    } else if (lock_password_hasher_matches(password, &v)) {
        m->failed = 0;
        m->retry_at_ms = 0;
        result = PRIVACY_CHECK_OK;
    } else {
        m->failed++;
        int64_t wait = idle_lock_policy_wait_ms(m->failed);
        m->retry_at_ms = now_ms + wait;
        if (wait_ms) *wait_ms = wait;
        result = PRIVACY_CHECK_WRONG;
    }
    lock_verifier_wipe(&v);
    passphrase_wipe(password);
    return result;
}

int privacy_manager_biometrics_available(PrivacyManager *m) {
    if (m->deps.settings && !m->deps.settings->private_touch_id) return 0;
    return m->deps.biometrics->available(m->deps.biometrics);
}

int privacy_manager_begin_biometric(PrivacyManager *m, UnlockPurpose purpose) {
    if (!privacy_manager_biometrics_available(m)) return -1;
    return m->deps.biometrics->begin(m->deps.biometrics, unlock_purpose_reason(purpose));
}

BiometricResult privacy_manager_poll_biometric(PrivacyManager *m) {
    return m->deps.biometrics->poll(m->deps.biometrics);
}

void privacy_manager_cancel_biometric(PrivacyManager *m) {
    m->deps.biometrics->cancel(m->deps.biometrics);
}
