#include "infrastructure/touch_id_authenticator.h"

#import <Foundation/Foundation.h>
#import <LocalAuthentication/LocalAuthentication.h>

#include <stdatomic.h>
#include <stdlib.h>

/* One request's outcome. The reply block runs on a queue of
 * LocalAuthentication's own and keeps this object alive, so a reply that
 * comes after a cancel, or after tawk has let go, never writes to freed
 * memory. poll reads it on the main thread; no run loop is needed. */
@interface TawkTouchIdOutcome : NSObject {
@public
    _Atomic int result;          /* a BiometricResult */
}
@end

@implementation TawkTouchIdOutcome
@end

typedef struct TouchIdState {
    void *outcome;               /* the TawkTouchIdOutcome of the last request, retained; NULL before any */
    void *context;               /* the LAContext of the request, retained */
} TouchIdState;

static TouchIdState *state_of(IBiometricAuthenticator *self) { return (TouchIdState *)self->ctx; }

static BiometricResult map_error(NSError *error) {
    switch (error ? error.code : LAErrorAuthenticationFailed) {
    case LAErrorUserFallback:          return BIOMETRIC_FALLBACK;
    case LAErrorUserCancel:
    case LAErrorSystemCancel:
    case LAErrorAppCancel:             return BIOMETRIC_CANCELLED;
    case LAErrorAuthenticationFailed:  return BIOMETRIC_FAILED;
    default:                           return BIOMETRIC_UNAVAILABLE;   /* not fitted, not enrolled, locked out, no screen */
    }
}

static TawkTouchIdOutcome *outcome_of(TouchIdState *s) { return (__bridge TawkTouchIdOutcome *)s->outcome; }

static void settle(TouchIdState *s, BiometricResult result) {
    TawkTouchIdOutcome *o = outcome_of(s);
    if (!o) return;
    int expected = BIOMETRIC_PENDING;
    atomic_compare_exchange_strong(&o->result, &expected, (int)result);
}

static void new_outcome(TouchIdState *s, BiometricResult result) {
    if (s->outcome) { TawkTouchIdOutcome *old = (__bridge_transfer TawkTouchIdOutcome *)s->outcome; (void)old; }
    TawkTouchIdOutcome *o = [[TawkTouchIdOutcome alloc] init];
    atomic_init(&o->result, (int)result);
    s->outcome = (__bridge_retained void *)o;
}

static void release_context(TouchIdState *s) {
    if (!s->context) return;
    LAContext *context = (__bridge_transfer LAContext *)s->context;
    s->context = NULL;
    [context invalidate];
}

static int touch_available(IBiometricAuthenticator *self) {
    (void)self;
    if (getenv("SSH_CONNECTION") || getenv("SSH_TTY")) return 0;
    @autoreleasepool {
        LAContext *context = [[LAContext alloc] init];
        return [context canEvaluatePolicy:LAPolicyDeviceOwnerAuthenticationWithBiometrics error:nil] ? 1 : 0;
    }
}

static int touch_begin(IBiometricAuthenticator *self, const char *reason) {
    TouchIdState *s = state_of(self);
    settle(s, BIOMETRIC_CANCELLED);
    release_context(s);
    if (!touch_available(self)) { new_outcome(s, BIOMETRIC_UNAVAILABLE); return -1; }
    new_outcome(s, BIOMETRIC_PENDING);
    @autoreleasepool {
        LAContext *context = [[LAContext alloc] init];
        context.localizedFallbackTitle = @"Use tawk password";
        NSString *why = [NSString stringWithUTF8String:reason && reason[0] ? reason : "open a private chat"];
        s->context = (__bridge_retained void *)context;
        TawkTouchIdOutcome *outcome = outcome_of(s);
        [context evaluatePolicy:LAPolicyDeviceOwnerAuthenticationWithBiometrics localizedReason:why
                          reply:^(BOOL success, NSError *error) {
            int expected = BIOMETRIC_PENDING;
            atomic_compare_exchange_strong(&outcome->result, &expected, success ? BIOMETRIC_OK : (int)map_error(error));
        }];
    }
    return 0;
}

static BiometricResult touch_poll(IBiometricAuthenticator *self) {
    TawkTouchIdOutcome *o = outcome_of(state_of(self));
    return o ? (BiometricResult)atomic_load(&o->result) : BIOMETRIC_UNAVAILABLE;
}

static void touch_cancel(IBiometricAuthenticator *self) {
    TouchIdState *s = state_of(self);
    settle(s, BIOMETRIC_CANCELLED);
    release_context(s);
}

static void touch_destroy(IBiometricAuthenticator *self) {
    if (!self) return;
    TouchIdState *s = state_of(self);
    touch_cancel(self);
    if (s->outcome) { TawkTouchIdOutcome *last = (__bridge_transfer TawkTouchIdOutcome *)s->outcome; (void)last; s->outcome = NULL; }
    free(s);
    free(self);
}

IBiometricAuthenticator *touch_id_authenticator_create(void) {
    IBiometricAuthenticator *a = calloc(1, sizeof(*a));
    TouchIdState *s = calloc(1, sizeof(*s));
    if (!a || !s) { free(a); free(s); return NULL; }
    a->ctx = s;
    a->available = touch_available;
    a->begin = touch_begin;
    a->poll = touch_poll;
    a->cancel = touch_cancel;
    a->destroy = touch_destroy;
    return a;
}
