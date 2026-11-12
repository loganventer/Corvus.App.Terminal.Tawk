#ifndef APP_INFRASTRUCTURE_TOUCH_ID_AUTHENTICATOR_H
#define APP_INFRASTRUCTURE_TOUCH_ID_AUTHENTICATOR_H

#include "contracts/i_biometric_authenticator.h"

/* Touch ID through macOS LocalAuthentication. It is available only when the
 * Mac can check a finger and tawk is not reached over SSH, where the system
 * sheet would appear on a screen the person typing cannot see. Built only on
 * macOS with BIOMETRICS=1. */
IBiometricAuthenticator *touch_id_authenticator_create(void);

#endif
