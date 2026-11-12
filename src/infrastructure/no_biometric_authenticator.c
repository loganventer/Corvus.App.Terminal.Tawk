#include "infrastructure/no_biometric_authenticator.h"

#include <stdlib.h>

static int             none_available(IBiometricAuthenticator *self) { (void)self; return 0; }
static int             none_begin(IBiometricAuthenticator *self, const char *reason) { (void)self; (void)reason; return -1; }
static BiometricResult none_poll(IBiometricAuthenticator *self) { (void)self; return BIOMETRIC_UNAVAILABLE; }
static void            none_cancel(IBiometricAuthenticator *self) { (void)self; }
static void            none_destroy(IBiometricAuthenticator *self) { free(self); }

IBiometricAuthenticator *no_biometric_authenticator_create(void) {
    IBiometricAuthenticator *a = calloc(1, sizeof(*a));
    if (!a) return NULL;
    a->available = none_available;
    a->begin = none_begin;
    a->poll = none_poll;
    a->cancel = none_cancel;
    a->destroy = none_destroy;
    return a;
}
