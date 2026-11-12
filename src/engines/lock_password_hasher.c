#include "engines/lock_password_hasher.h"
#include "utilities/secure_zero.h"
#include "utilities/sha256.h"

#include <string.h>

#define KEY_NOTE "tawk chat lock"          /* keeps this key apart from any other made from the same password */

static void derive(const Passphrase *p, const uint8_t salt[LOCK_VERIFIER_SALT_BYTES], uint32_t iterations, uint8_t out[LOCK_VERIFIER_HASH_BYTES]) {
    uint8_t salted[sizeof(KEY_NOTE) - 1 + LOCK_VERIFIER_SALT_BYTES];
    memcpy(salted, KEY_NOTE, sizeof(KEY_NOTE) - 1);
    memcpy(salted + sizeof(KEY_NOTE) - 1, salt, LOCK_VERIFIER_SALT_BYTES);
    pbkdf2_hmac_sha256(p->text, p->length, salted, sizeof(salted), iterations, out, LOCK_VERIFIER_HASH_BYTES);
    secure_zero(salted, sizeof(salted));
}

void lock_password_hasher_derive(const Passphrase *password, const uint8_t salt[LOCK_VERIFIER_SALT_BYTES], LockVerifier *out) {
    lock_verifier_wipe(out);
    memcpy(out->salt, salt, LOCK_VERIFIER_SALT_BYTES);
    out->iterations = LOCK_PASSWORD_ITERATIONS;
    derive(password, salt, out->iterations, out->hash);
}

int lock_password_hasher_matches(const Passphrase *password, const LockVerifier *verifier) {
    if (!password || password->length == 0 || !verifier || verifier->iterations == 0) return 0;
    uint8_t key[LOCK_VERIFIER_HASH_BYTES];
    derive(password, verifier->salt, verifier->iterations, key);
    int same = sha256_equal(key, verifier->hash, sizeof(key));
    secure_zero(key, sizeof(key));
    return same;
}
