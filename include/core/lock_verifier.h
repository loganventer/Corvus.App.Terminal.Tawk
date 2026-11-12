#ifndef APP_CORE_LOCK_VERIFIER_H
#define APP_CORE_LOCK_VERIFIER_H

#include <stdint.h>

#define LOCK_VERIFIER_SALT_BYTES 16
#define LOCK_VERIFIER_HASH_BYTES 32

/* What tawk keeps to check the lock password for private chats: a random
 * salt, the PBKDF2 iteration count and the derived hash. The password itself
 * is never stored. */
typedef struct LockVerifier {
    uint8_t  salt[LOCK_VERIFIER_SALT_BYTES];
    uint32_t iterations;
    uint8_t  hash[LOCK_VERIFIER_HASH_BYTES];
} LockVerifier;

/* Overwrites it so the hash does not linger in memory. */
void lock_verifier_wipe(LockVerifier *verifier);

#endif
