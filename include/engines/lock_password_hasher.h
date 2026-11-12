#ifndef APP_ENGINES_LOCK_PASSWORD_HASHER_H
#define APP_ENGINES_LOCK_PASSWORD_HASHER_H

#include <stdint.h>

#include "core/lock_verifier.h"
#include "core/passphrase.h"

#define LOCK_PASSWORD_ITERATIONS 600000

/* Turns the lock password into what is kept: PBKDF2-HMAC-SHA256 over the
 * caller's random salt, with a label so this key is never the same as one
 * made from the same words for anything else. */
void lock_password_hasher_derive(const Passphrase *password, const uint8_t salt[LOCK_VERIFIER_SALT_BYTES], LockVerifier *out);
/* Non-zero when `password` is the one `verifier` was made from. */
int  lock_password_hasher_matches(const Passphrase *password, const LockVerifier *verifier);

#endif
