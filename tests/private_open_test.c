/* Every way a chat opens asks first when it is private: nothing is loaded
 * and no read mark goes out until Touch ID or the password lets it. */
#include "tui_harness.h"

static int failures;
#define CHECK(cond, what) do { if (!(cond)) { printf("FAIL: %s\n", what); failures++; } } while (0)

#define DIARY "27820000001@s.whatsapp.net"
#define MOM   "27820000002@s.whatsapp.net"

static void opening_by_jid(void) {
    Harness h;
    if (harness_open(&h, 0, "letmein") != 0) { printf("FAIL: the harness opens\n"); failures++; return; }
    harness_chat(&h, 0, DIARY, "Diary", 1);
    harness_chat(&h, 0, MOM, "Mom", 0);

    tui_app_open_chat(h.app, MOM);
    CHECK(!strcmp(harness_open_jid(&h), MOM), "a public chat opens at once");
    tui_app_open_chat(h.app, DIARY);
    CHECK(!strcmp(harness_open_jid(&h), MOM) && private_chat_guard_busy(&h.app->privacy),
          "a private one is not opened: the password is asked for");
    harness_type(&h, "wrong");
    CHECK(!strcmp(harness_open_jid(&h), MOM), "a wrong password still opens nothing");
    harness_type(&h, "letmein");
    CHECK(!strcmp(harness_open_jid(&h), DIARY) && !private_chat_guard_busy(&h.app->privacy), "the right one opens it");
    const Chat *open = messaging_manager_open_chat_info(h.app->deps.messaging);
    CHECK(open && !tui_app_private_veiled(h.app, open), "and it shows as it is");
    tui_app_open_chat(h.app, MOM);
    CHECK(tui_app_private_veiled(h.app, &(Chat){ .jid = DIARY, .is_private = 1 }), "leaving it locks it again");

    tui_app_open_chat(h.app, DIARY);
    tui_app_private_key(h.app, 0, 27);
    CHECK(!strcmp(harness_open_jid(&h), MOM) && !private_chat_guard_busy(&h.app->privacy), "Esc leaves the chat you were in");
    harness_close(&h);
}

static void opening_a_merged_row(void) {
    Harness h;
    if (harness_open(&h, 1, "letmein") != 0) { printf("FAIL: the harness opens\n"); failures++; return; }
    harness_chat(&h, 0, DIARY, "Diary", 1);
    harness_chat(&h, 1, DIARY, "Diary", 0);
    Chat row;
    chat_init(&row, DIARY);
    str_copy(row.name, sizeof(row.name), "Diary");
    row.is_private = 1;                 /* private on one account makes the merged row private */
    row.account = 1;
    row.accounts = 3;                   /* on both accounts */
    tui_app_open_row(h.app, &row);
    CHECK(h.privacy.touch.begun == 1 && harness_open_jid(&h)[0] == '\0' &&
          !messaging_manager_open_jid(h.accounts[1].messaging)[0], "a merged private row opens on neither account before Touch ID");
    h.privacy.touch.next = BIOMETRIC_OK;
    tui_app_private_tick(h.app, 100);
    CHECK(!strcmp(harness_open_jid(&h), DIARY) && !strcmp(messaging_manager_open_jid(h.accounts[1].messaging), DIARY),
          "after it, both accounts' sides open");
    CHECK(private_chat_session_is_unlocked(&h.app->privacy.session, 1, DIARY) && private_chat_session_is_unlocked(&h.app->privacy.session, 2, DIARY),
          "and both are unlocked together");
    harness_close(&h);
}

int main(void) {
    opening_by_jid();
    opening_a_merged_row();
    if (failures) { printf("%d failure(s)\n", failures); return 1; }
    printf("ok: a private chat opens only after an unlock, on every account it is merged across, and locks when you leave\n");
    return 0;
}
