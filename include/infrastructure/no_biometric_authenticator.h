#ifndef APP_INFRASTRUCTURE_NO_BIOMETRIC_AUTHENTICATOR_H
#define APP_INFRASTRUCTURE_NO_BIOMETRIC_AUTHENTICATOR_H

#include "contracts/i_biometric_authenticator.h"

/* Where there is no Touch ID (Linux, WSL, or a build with BIOMETRICS=0):
 * never available, so a private chat asks for the lock password at once. */
IBiometricAuthenticator *no_biometric_authenticator_create(void);

#endif
