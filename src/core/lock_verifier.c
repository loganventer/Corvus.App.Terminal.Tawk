#include "core/lock_verifier.h"
#include "utilities/secure_zero.h"

void lock_verifier_wipe(LockVerifier *verifier) {
    secure_zero(verifier, sizeof(*verifier));
}
