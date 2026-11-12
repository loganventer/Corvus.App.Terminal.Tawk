#ifndef APP_CONTRACTS_I_BIOMETRIC_AUTHENTICATOR_H
#define APP_CONTRACTS_I_BIOMETRIC_AUTHENTICATOR_H

#include "core/biometric_result.h"

/* Touch ID, or whatever stands in for it. It is asynchronous: begin puts the
 * system sheet up and returns at once, and the caller polls once a frame, so
 * the terminal keeps drawing while the sheet is up. One request at a time. */
typedef struct IBiometricAuthenticator {
    void *ctx;
    /* Non-zero when a request could succeed here: the hardware is there, a
     * finger is enrolled and there is a screen to show the sheet on. */
    int             (*available)(struct IBiometricAuthenticator *self);
    /* Starts a request. `reason` ends the system's sentence ("open a private
     * chat") and never names a chat. Returns 0, or -1 when it cannot start. */
    int             (*begin)(struct IBiometricAuthenticator *self, const char *reason);
    /* BIOMETRIC_PENDING until the request has settled, then how it ended. */
    BiometricResult (*poll)(struct IBiometricAuthenticator *self);
    /* Takes the sheet down; poll then answers BIOMETRIC_CANCELLED. */
    void            (*cancel)(struct IBiometricAuthenticator *self);
    void            (*destroy)(struct IBiometricAuthenticator *self);
} IBiometricAuthenticator;

#endif
