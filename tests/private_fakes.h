/* A lock password store and a Touch ID that are only memory, for the
 * private chat tests. Header-only: each test includes it once. */
#ifndef TESTS_PRIVATE_FAKES_H
#define TESTS_PRIVATE_FAKES_H

#include "contracts/i_biometric_authenticator.h"
#include "contracts/i_lock_secret_store.h"
#include "core/settings.h"
#include "managers/privacy_manager.h"

#include <stdio.h>
#include <string.h>

typedef struct FakeStore { ILockSecretStore base; LockVerifier kept; int has; } FakeStore;
static int  fake_store_load(ILockSecretStore *s, LockVerifier *out) { FakeStore *f = (FakeStore *)s; if (!f->has) return 0; *out = f->kept; return 1; }
static int  fake_store_save(ILockSecretStore *s, const LockVerifier *v) { FakeStore *f = (FakeStore *)s; f->kept = *v; f->has = 1; return 0; }
static int  fake_store_clear(ILockSecretStore *s) { ((FakeStore *)s)->has = 0; return 0; }
static void fake_store_destroy(ILockSecretStore *s) { (void)s; }

typedef struct FakeTouch { IBiometricAuthenticator base; int available; int begun; int cancelled; BiometricResult next; } FakeTouch;
static int             fake_touch_available(IBiometricAuthenticator *a) { return ((FakeTouch *)a)->available; }
static int             fake_touch_begin(IBiometricAuthenticator *a, const char *reason) { (void)reason; FakeTouch *t = (FakeTouch *)a; t->begun++; t->next = BIOMETRIC_PENDING; return 0; }
static BiometricResult fake_touch_poll(IBiometricAuthenticator *a) { return ((FakeTouch *)a)->next; }
static void            fake_touch_cancel(IBiometricAuthenticator *a) { FakeTouch *t = (FakeTouch *)a; t->cancelled++; if (t->next == BIOMETRIC_PENDING) t->next = BIOMETRIC_CANCELLED; }
static void            fake_touch_destroy(IBiometricAuthenticator *a) { (void)a; }

typedef struct FakePrivacy {
    FakeStore       store;
    FakeTouch       touch;
    Settings        settings;
    PrivacyManager *manager;
} FakePrivacy;

/* A privacy manager over the fakes, with Touch ID there or not and a lock password set or not. */
static void fake_privacy_init(FakePrivacy *p, int touch_id, const char *password) {
    memset(p, 0, sizeof(*p));
    p->store.base = (ILockSecretStore){ NULL, fake_store_load, fake_store_save, fake_store_clear, fake_store_destroy };
    p->touch.base = (IBiometricAuthenticator){ NULL, fake_touch_available, fake_touch_begin, fake_touch_poll, fake_touch_cancel, fake_touch_destroy };
    p->touch.available = touch_id;
    p->touch.next = BIOMETRIC_UNAVAILABLE;
    settings_set_defaults(&p->settings);
    PrivacyManagerDeps deps = { &p->store.base, &p->touch.base, &p->settings };
    p->manager = privacy_manager_create(&deps);
    if (password) {
        Passphrase a, b;
        passphrase_set(&a, password);
        passphrase_set(&b, password);
        privacy_manager_set_password(p->manager, &a, &b);
    }
}

static void fake_privacy_dispose(FakePrivacy *p) { privacy_manager_destroy(p->manager); }

#endif
