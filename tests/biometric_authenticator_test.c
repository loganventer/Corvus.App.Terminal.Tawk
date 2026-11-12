/* Where there is no Touch ID, a private chat goes straight to the password. */
#include "infrastructure/no_biometric_authenticator.h"

#include <stdio.h>

static int failures;
#define CHECK(cond, what) do { if (!(cond)) { printf("FAIL: %s\n", what); failures++; } } while (0)

int main(void) {
    IBiometricAuthenticator *a = no_biometric_authenticator_create();
    CHECK(a != NULL, "it is made");
    if (a) {
        CHECK(a->available(a) == 0, "it is never available");
        CHECK(a->poll(a) == BIOMETRIC_UNAVAILABLE, "and says so before a request");
        a->begin(a, "open a private chat");
        CHECK(a->poll(a) == BIOMETRIC_UNAVAILABLE, "and after one");
        a->cancel(a);
        CHECK(a->poll(a) == BIOMETRIC_UNAVAILABLE, "and after a cancel");
        a->destroy(a);
    }
    if (failures) { printf("%d failure(s)\n", failures); return 1; }
    printf("ok: without Touch ID a private chat asks for the lock password at once\n");
    return 0;
}
