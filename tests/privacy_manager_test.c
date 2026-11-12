/* The lock password and Touch ID behind private chats, with a store and a
 * Touch ID that are only memory. */
#include "managers/privacy_manager.h"
#include "core/settings.h"

#include <stdio.h>
#include <string.h>

static int failures;
#define CHECK(cond, what) do { if (!(cond)) { printf("FAIL: %s\n", what); failures++; } } while (0)

typedef struct FakeStore { ILockSecretStore base; LockVerifier kept; int has; } FakeStore;
static int  fs_load(ILockSecretStore *s, LockVerifier *out) { FakeStore *f = (FakeStore *)s; if (!f->has) return 0; *out = f->kept; return 1; }
static int  fs_save(ILockSecretStore *s, const LockVerifier *v) { FakeStore *f = (FakeStore *)s; f->kept = *v; f->has = 1; return 0; }
static int  fs_clear(ILockSecretStore *s) { ((FakeStore *)s)->has = 0; return 0; }
static void fs_destroy(ILockSecretStore *s) { (void)s; }

typedef struct FakeTouch { IBiometricAuthenticator base; int available; int begun; char reason[128]; BiometricResult next; int cancelled; } FakeTouch;
static int             ft_available(IBiometricAuthenticator *a) { return ((FakeTouch *)a)->available; }
static int             ft_begin(IBiometricAuthenticator *a, const char *reason) { FakeTouch *t = (FakeTouch *)a; t->begun++; snprintf(t->reason, sizeof(t->reason), "%s", reason); return 0; }
static BiometricResult ft_poll(IBiometricAuthenticator *a) { return ((FakeTouch *)a)->next; }
static void            ft_cancel(IBiometricAuthenticator *a) { ((FakeTouch *)a)->cancelled++; }
static void            ft_destroy(IBiometricAuthenticator *a) { (void)a; }

static Passphrase pw(const char *text) { Passphrase p; memset(&p, 0, sizeof(p)); passphrase_set(&p, text); return p; }

int main(void) {
    FakeStore store = { { NULL, fs_load, fs_save, fs_clear, fs_destroy }, { { 0 }, 0, { 0 } }, 0 };
    FakeTouch touch = { { NULL, ft_available, ft_begin, ft_poll, ft_cancel, ft_destroy }, 1, 0, "", BIOMETRIC_PENDING, 0 };
    Settings settings;
    settings_set_defaults(&settings);
    PrivacyManagerDeps deps = { &store.base, &touch.base, &settings };
    PrivacyManager *m = privacy_manager_create(&deps);
    CHECK(m != NULL, "it is made");
    if (!m) return 1;

    int64_t wait = -1;
    Passphrase p = pw("open sesame");
    CHECK(!privacy_manager_has_password(m), "there is no lock password at first");
    CHECK(privacy_manager_check_password(m, &p, 0, &wait) == PRIVACY_CHECK_NO_PASSWORD, "and nothing can be checked against none");
    CHECK(p.length == 0 && p.text[0] == '\0', "what was typed is wiped even then");

    Passphrase a = pw("open sesame"), b = pw("open sesamf");
    CHECK(privacy_manager_set_password(m, &a, &b) == LOCK_PASSWORD_MISMATCH && !privacy_manager_has_password(m), "two different passwords set nothing");
    CHECK(a.length == 0 && b.length == 0, "and both are wiped");
    a = pw("open sesame");
    b = pw("open sesame");
    CHECK(privacy_manager_set_password(m, &a, &b) == LOCK_PASSWORD_OK && privacy_manager_has_password(m), "the same twice sets it");
    CHECK(store.kept.iterations > 0 && memcmp(store.kept.hash, "open sesame", 11) != 0, "and what is kept is a hash");
    uint8_t first_salt[LOCK_VERIFIER_SALT_BYTES];
    memcpy(first_salt, store.kept.salt, sizeof(first_salt));

    p = pw("open sesame");
    CHECK(privacy_manager_check_password(m, &p, 1000, &wait) == PRIVACY_CHECK_OK && wait == 0, "the right password opens");
    CHECK(p.length == 0, "and is wiped");

    int wrong_free = 1;
    for (int i = 0; i < 2; i++) {
        p = pw("nope");
        if (privacy_manager_check_password(m, &p, 2000, &wait) != PRIVACY_CHECK_WRONG || wait != 0) wrong_free = 0;
    }
    CHECK(wrong_free, "the first wrong tries come with no wait");
    p = pw("nope");
    CHECK(privacy_manager_check_password(m, &p, 2000, &wait) == PRIVACY_CHECK_WRONG && wait == 1000, "the third waits a second, as the app lock does");
    p = pw("open sesame");
    CHECK(privacy_manager_check_password(m, &p, 2500, &wait) == PRIVACY_CHECK_WAIT && wait == 500, "and even the right one is not looked at until then");
    CHECK(p.length == 0, "though it is still wiped");
    p = pw("nope");
    CHECK(privacy_manager_check_password(m, &p, 3000, &wait) == PRIVACY_CHECK_WRONG && wait == 2000, "the wait doubles");
    p = pw("open sesame");
    CHECK(privacy_manager_check_password(m, &p, 5000, &wait) == PRIVACY_CHECK_OK, "the right one after the wait opens");
    p = pw("nope");
    CHECK(privacy_manager_check_password(m, &p, 5000, &wait) == PRIVACY_CHECK_WRONG && wait == 0, "and forgets the count");

    a = pw("new words");
    b = pw("new words");
    CHECK(privacy_manager_set_password(m, &a, &b) == LOCK_PASSWORD_OK && memcmp(first_salt, store.kept.salt, sizeof(first_salt)) != 0,
          "a new password gets a fresh salt");
    p = pw("open sesame");
    CHECK(privacy_manager_check_password(m, &p, 6000, &wait) == PRIVACY_CHECK_WRONG, "and the old one no longer opens");

    CHECK(privacy_manager_biometrics_available(m), "Touch ID is offered where it is there and switched on");
    CHECK(privacy_manager_begin_biometric(m, UNLOCK_FOR_EXPORT) == 0 && touch.begun == 1 && !strcmp(touch.reason, "export a private chat"),
          "it is asked with the purpose's reason");
    CHECK(privacy_manager_poll_biometric(m) == BIOMETRIC_PENDING, "and is pending until it answers");
    touch.next = BIOMETRIC_FALLBACK;
    CHECK(privacy_manager_poll_biometric(m) == BIOMETRIC_FALLBACK, "its answer is passed on");
    privacy_manager_cancel_biometric(m);
    CHECK(touch.cancelled == 1, "a cancel reaches it");
    settings.private_touch_id = 0;
    CHECK(!privacy_manager_biometrics_available(m) && privacy_manager_begin_biometric(m, UNLOCK_FOR_OPEN) == -1 && touch.begun == 1,
          "with the setting off it is never asked");
    settings.private_touch_id = 1;
    touch.available = 0;
    CHECK(!privacy_manager_biometrics_available(m), "nor where the Mac cannot use it");

    privacy_manager_destroy(m);
    if (failures) { printf("%d failure(s)\n", failures); return 1; }
    printf("ok: the lock password is set twice, checked with waits after wrong tries, and Touch ID is asked only when it can be\n");
    return 0;
}
