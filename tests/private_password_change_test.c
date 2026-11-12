/* /lockpassword: Touch ID or the current password first, then a new one
 * twice. Touch ID is also how a forgotten password is replaced. */
#include "tui_harness.h"

static int failures;
#define CHECK(cond, what) do { if (!(cond)) { printf("FAIL: %s\n", what); failures++; } } while (0)

static void keys(Harness *h, const char *text) {
    for (const char *p = text; *p; p++) tui_app_private_key(h->app, 0, (unsigned char)*p);
}

static int opens_with(Harness *h, const char *password) {
    Passphrase p;
    passphrase_set(&p, password);
    int64_t wait = 0;
    return privacy_manager_check_password(h->privacy.manager, &p, INT64_C(1) << 62, &wait) == PRIVACY_CHECK_OK;
}

static void touch_id_then_new(void) {
    Harness h;
    if (harness_open(&h, 1, "forgotten") != 0) { printf("FAIL: the harness opens\n"); failures++; return; }
    LockVerifier before = h.privacy.store.kept;
    tui_app_change_lock_password(h.app);
    CHECK(h.privacy.touch.begun == 1 && h.app->privacy.unlock.open, "/lockpassword asks Touch ID first");
    h.privacy.touch.next = BIOMETRIC_OK;
    tui_app_private_tick(h.app, 10);
    CHECK(h.app->privacy.password.open && !h.app->privacy.password.first, "after it, the new password is asked for");
    keys(&h, "brand new\rbrand new\r");
    CHECK(!private_chat_guard_busy(&h.app->privacy) && memcmp(&before, &h.privacy.store.kept, sizeof(before)) != 0,
          "and replaces the stored verifier");
    CHECK(opens_with(&h, "brand new") && !opens_with(&h, "forgotten"), "the new password works and the forgotten one does not");
    harness_close(&h);
}

static void wrong_current_password(void) {
    Harness h;
    if (harness_open(&h, 0, "current") != 0) { printf("FAIL: the harness opens\n"); failures++; return; }
    LockVerifier before = h.privacy.store.kept;
    tui_app_change_lock_password(h.app);
    CHECK(h.app->privacy.unlock.open && h.app->privacy.unlock.state == PRIVATE_UNLOCK_PASSWORD, "without Touch ID it asks for the current password");
    keys(&h, "wrong\r");
    CHECK(h.app->privacy.unlock.open && !h.app->privacy.password.open, "a wrong one does not get to the new password");
    keys(&h, "\x1b");
    CHECK(!memcmp(&before, &h.privacy.store.kept, sizeof(before)) && opens_with(&h, "current"), "and the password is unchanged");
    harness_close(&h);
}

int main(void) {
    touch_id_then_new();
    wrong_current_password();
    if (failures) { printf("%d failure(s)\n", failures); return 1; }
    printf("ok: /lockpassword asks Touch ID or the current password, then sets a new one; a wrong one changes nothing\n");
    return 0;
}
