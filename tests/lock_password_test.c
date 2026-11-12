/* The lock password for private chats: how it is kept and checked. */
#include "core/lock_verifier.h"
#include "core/passphrase.h"
#include "engines/lock_password_hasher.h"

#include <stdio.h>
#include <string.h>

static int failures;
#define CHECK(cond, what) do { if (!(cond)) { printf("FAIL: %s\n", what); failures++; } } while (0)

static Passphrase pw(const char *text) {
    Passphrase p;
    memset(&p, 0, sizeof(p));
    if (text[0]) passphrase_set(&p, text);
    return p;
}

static void hasher(void) {
    const uint8_t salt_a[LOCK_VERIFIER_SALT_BYTES] = { 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16 };
    const uint8_t salt_b[LOCK_VERIFIER_SALT_BYTES] = { 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1 };
    Passphrase right = pw("correct horse"), wrong = pw("correct horsf"), empty = pw("");
    LockVerifier a, b;
    lock_password_hasher_derive(&right, salt_a, &a);
    lock_password_hasher_derive(&right, salt_b, &b);
    CHECK(a.iterations == LOCK_PASSWORD_ITERATIONS && !memcmp(a.salt, salt_a, sizeof(salt_a)), "the salt and the work are kept with the hash");
    CHECK(lock_password_hasher_matches(&right, &a), "the right password matches");
    CHECK(!lock_password_hasher_matches(&wrong, &a), "a wrong one does not");
    CHECK(!lock_password_hasher_matches(&empty, &a), "nor does an empty one");
    CHECK(memcmp(a.hash, b.hash, sizeof(a.hash)) != 0, "two salts give different hashes");
    CHECK(memcmp(a.hash, right.text, right.length) != 0, "the password itself is not what is kept");
    lock_verifier_wipe(&a);
    CHECK(a.iterations == 0 && !lock_password_hasher_matches(&right, &a), "a wiped verifier matches nothing");
    passphrase_wipe(&right);
    passphrase_wipe(&wrong);
}

int main(void) {
    hasher();
    if (failures) { printf("%d failure(s)\n", failures); return 1; }
    printf("ok: the lock password is kept as a salted hash and checked against it\n");
    return 0;
}
