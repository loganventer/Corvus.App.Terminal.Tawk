#ifndef APP_MANAGERS_PRIVACY_MANAGER_DEPS_H
#define APP_MANAGERS_PRIVACY_MANAGER_DEPS_H

#include "contracts/i_biometric_authenticator.h"
#include "contracts/i_lock_secret_store.h"
#include "core/settings.h"

/* What the privacy manager depends on, injected by the composition root. */
typedef struct PrivacyManagerDeps {
    ILockSecretStore        *secrets;     /* the lock password's verifier */
    IBiometricAuthenticator *biometrics;  /* Touch ID, or the stand-in that never is */
    const Settings          *settings;    /* the live settings: private_touch_id */
} PrivacyManagerDeps;

#endif
