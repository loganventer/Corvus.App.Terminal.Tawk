/* The lock password for private chats: how it is kept and checked. */
#include "core/lock_verifier.h"
#include "core/passphrase.h"
#include "engines/lock_password_hasher.h"
#include "engines/lock_password_rule.h"
#include "resource_access/sqlite_database.h"
#include "resource_access/sqlite_lock_secret_store.h"

#include <stdlib.h>
#include <unistd.h>

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

static void rule(void) {
    Passphrase empty = pw(""), three = pw("abc"), four = pw("abcd"), other = pw("abce");
    Passphrase multibyte = pw("\xC3\xA9\xC3\xA9\xC3\xA9\xC3\xA9");   /* four letters, eight bytes */
    Passphrase three_multibyte = pw("\xC3\xA9\xC3\xA9\xC3\xA9");
    Passphrase too_long;
    memset(&too_long, 0, sizeof(too_long));
    memset(too_long.text, 'a', PASSPHRASE_MAX);
    too_long.length = PASSPHRASE_MAX + 1;                        /* longer than any field can hold */
    CHECK(lock_password_rule_check(&empty, &empty) == LOCK_PASSWORD_EMPTY, "an empty password is refused");
    CHECK(lock_password_rule_check(&three, &three) == LOCK_PASSWORD_TOO_SHORT, "three characters are too few");
    CHECK(lock_password_rule_check(&three_multibyte, &three_multibyte) == LOCK_PASSWORD_TOO_SHORT, "and are counted as letters, not bytes");
    CHECK(lock_password_rule_check(&too_long, &too_long) == LOCK_PASSWORD_TOO_LONG, "a password longer than a passphrase is refused");
    CHECK(lock_password_rule_check(&four, &other) == LOCK_PASSWORD_MISMATCH, "the two must be the same");
    CHECK(lock_password_rule_check(&four, &four) == LOCK_PASSWORD_OK, "four characters typed twice are fine");
    CHECK(lock_password_rule_check(&multibyte, &multibyte) == LOCK_PASSWORD_OK, "four letters beyond ASCII are fine too");
    for (int p = LOCK_PASSWORD_EMPTY; p <= LOCK_PASSWORD_MISMATCH; p++)
        CHECK(lock_password_problem_text((LockPasswordProblem)p)[0] != '\0', "every problem has words for the dialog");
    CHECK(lock_password_problem_text(LOCK_PASSWORD_OK)[0] == '\0', "and no problem says nothing");
}

static void store(void) {
    char dir[] = "/tmp/tawk-lock-secret-XXXXXX";
    if (!mkdtemp(dir)) { printf("FAIL: mkdtemp\n"); failures++; return; }
    char path[256];
    snprintf(path, sizeof(path), "%s/tawk.db", dir);
    sqlite3 *db = sqlite_database_open(path, NULL);
    ILockSecretStore *s = db ? sqlite_lock_secret_store_create(db) : NULL;
    CHECK(s != NULL, "the store opens");
    if (s) {
        LockVerifier got, first, second;
        CHECK(s->load(s, &got) == 0, "there is no lock password at first");
        memset(&first, 0, sizeof(first));
        memset(first.salt, 0x11, sizeof(first.salt));
        memset(first.hash, 0x22, sizeof(first.hash));
        first.iterations = 600000;
        second = first;
        memset(second.salt, 0x33, sizeof(second.salt));
        second.iterations = 700000;
        CHECK(s->save(s, &first) == 0 && s->load(s, &got) == 1 && !memcmp(&got, &first, sizeof(got)), "what is saved loads back as it was");
        CHECK(s->save(s, &second) == 0 && s->load(s, &got) == 1 && !memcmp(&got, &second, sizeof(got)), "a second save replaces the first");
        CHECK(s->clear(s) == 0 && s->load(s, &got) == 0, "and clearing it leaves none");
        s->destroy(s);
    }
    sqlite_database_close(db);
    unlink(path);
    char cmd[128];
    snprintf(cmd, sizeof(cmd), "rm -rf %s", dir);
    if (system(cmd) != 0) printf("could not remove %s\n", dir);
}

int main(void) {
    hasher();
    rule();
    store();
    if (failures) { printf("%d failure(s)\n", failures); return 1; }
    printf("ok: the lock password is checked when it is set, kept as a salted hash on this computer and matched against it\n");
    return 0;
}
