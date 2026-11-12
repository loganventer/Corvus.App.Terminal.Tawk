/* The two dialogs of private chats: unlocking one, and setting the lock
 * password. They say what they are for, show how much was typed and never
 * what, and show nothing of the chat. */
#include "clients/tui/lock_password_dialog.h"
#include "clients/tui/private_unlock_dialog.h"
#include "clients/tui/tui_palette.h"

#include <locale.h>
#include <ncurses.h>
#include <stdio.h>
#include <string.h>

static int failures;
#define CHECK(cond, what) do { if (!(cond)) { printf("FAIL: %s\n", what); failures++; } } while (0)

static int on_screen(const char *words) {
    char line[512];
    for (int row = 0; row < LINES; row++) {
        if (mvinnstr(row, 0, line, sizeof(line) - 1) == ERR) continue;
        if (strstr(line, words)) return 1;
    }
    return 0;
}

static void type(PrivateUnlockDialog *d, const char *text, int64_t now) {
    for (const char *p = text; *p; p++) private_unlock_dialog_key(d, 0, (unsigned char)*p, now);
}

static void unlock_dialog(UiRect area) {
    PrivateUnlockDialog d;
    private_unlock_dialog_open(&d, UNLOCK_FOR_OPEN, 1);
    erase();
    private_unlock_dialog_render(&d, area, 0);
    CHECK(d.state == PRIVATE_UNLOCK_WAITING_TOUCH_ID && on_screen("Touch ID") && on_screen("To open a private chat"),
          "with Touch ID it waits for a finger and says what for");
    CHECK(private_unlock_dialog_key(&d, 0, 'x', 0) == PRIVATE_UNLOCK_KEY_NONE && d.field.length == 0, "keys are swallowed while the sheet is up");
    CHECK(private_unlock_dialog_key(&d, 0, '\t', 0) == PRIVATE_UNLOCK_KEY_USE_PASSWORD, "Tab asks for the password instead");

    private_unlock_dialog_show_password(&d);
    type(&d, "hunter22", 0);
    erase();
    private_unlock_dialog_render(&d, area, 0);
    CHECK(d.state == PRIVATE_UNLOCK_PASSWORD && on_screen("********") && !on_screen("hunter"),
          "the password field shows one mark a letter, never the letters");
    CHECK(private_unlock_dialog_key(&d, 0, '\r', 0) == PRIVATE_UNLOCK_KEY_SUBMIT, "Enter hands it over");
    Passphrase p;
    private_unlock_dialog_take(&d, &p);
    CHECK(!strcmp(p.text, "hunter22") && d.field.length == 0, "and the field is cleared");
    passphrase_wipe(&p);

    private_unlock_dialog_wrong(&d, 0, 0);
    erase();
    private_unlock_dialog_render(&d, area, 0);
    CHECK(on_screen("Wrong password"), "a wrong one says so");

    private_unlock_dialog_wrong(&d, 2000, 1000);
    type(&d, "again", 1500);
    erase();
    private_unlock_dialog_render(&d, area, 1500);
    CHECK(d.state == PRIVATE_UNLOCK_WAIT && on_screen("Try again in 2 s"), "after several it counts down the wait");
    CHECK(private_unlock_dialog_key(&d, 0, '\r', 1500) == PRIVATE_UNLOCK_KEY_CHANGED, "and Enter is not taken until it is over");
    CHECK(private_unlock_dialog_key(&d, 0, '\r', 3000) == PRIVATE_UNLOCK_KEY_SUBMIT && d.state == PRIVATE_UNLOCK_PASSWORD, "then it is");

    CHECK(private_unlock_dialog_key(&d, 0, 27, 3000) == PRIVATE_UNLOCK_KEY_CANCEL, "Esc cancels");
    private_unlock_dialog_close(&d);
    CHECK(!d.open && d.field.length == 0 && d.field.text[0] == '\0', "closing wipes what was typed");

    private_unlock_dialog_open(&d, UNLOCK_FOR_EXPORT, 0);
    erase();
    private_unlock_dialog_render(&d, area, 0);
    CHECK(d.state == PRIVATE_UNLOCK_PASSWORD && on_screen("To export a private chat") && on_screen("Type the lock password"),
          "without Touch ID it asks for the password at once");
}

static void password_dialog(UiRect area) {
    LockPasswordDialog d;
    lock_password_dialog_open(&d, 1);
    erase();
    lock_password_dialog_render(&d, area);
    CHECK(on_screen("Set a lock password"), "the first time it says it sets one");
    for (const char *c = "abcd"; *c; c++) lock_password_dialog_key(&d, 0, *c);
    CHECK(d.focus == 0 && d.fields[0].length == 4, "typing goes to the first field");
    CHECK(lock_password_dialog_key(&d, 0, '\t') == LOCK_PASSWORD_KEY_CHANGED && d.focus == 1, "Tab moves to the second");
    for (const char *c = "abce"; *c; c++) lock_password_dialog_key(&d, 0, *c);
    CHECK(d.fields[1].length == 4 && d.fields[0].length == 4, "which fills on its own");
    CHECK(lock_password_dialog_key(&d, 0, '\t') == LOCK_PASSWORD_KEY_CHANGED && d.focus == 0, "and Tab moves back");
    CHECK(lock_password_dialog_key(&d, 0, '\r') == LOCK_PASSWORD_KEY_CHANGED && d.focus == 1, "Enter on the first moves on");
    CHECK(lock_password_dialog_key(&d, 0, '\r') == LOCK_PASSWORD_KEY_SUBMIT, "Enter on the second hands both over");
    Passphrase a, b;
    lock_password_dialog_take(&d, &a, &b);
    CHECK(!strcmp(a.text, "abcd") && !strcmp(b.text, "abce") && d.fields[0].length == 0 && d.fields[1].length == 0, "both are handed over and cleared");
    passphrase_wipe(&a);
    passphrase_wipe(&b);
    lock_password_dialog_refused(&d, LOCK_PASSWORD_MISMATCH);
    erase();
    lock_password_dialog_render(&d, area);
    CHECK(on_screen("The two passwords are not the same") && !on_screen("abc"), "a mismatch is shown, never the passwords");
    for (const char *c = "secret"; *c; c++) lock_password_dialog_key(&d, 0, *c);
    lock_password_dialog_close(&d);
    CHECK(!d.open && d.fields[0].length == 0 && d.fields[0].text[0] == '\0', "closing wipes both fields");
    lock_password_dialog_open(&d, 0);
    erase();
    lock_password_dialog_render(&d, area);
    CHECK(on_screen("Change the lock password"), "through /lockpassword it says it changes it");
}

int main(void) {
    setlocale(LC_ALL, "");
    FILE *out = fopen("/dev/null", "w"), *in = fopen("/dev/null", "r");
    SCREEN *screen = out && in ? newterm("xterm-256color", out, in) : NULL;
    CHECK(screen != NULL, "a screen to draw on");
    if (!screen) return 1;
    tui_palette_init();
    resizeterm(30, 100);
    UiRect area = { 0, 0, 30, 100 };
    unlock_dialog(area);
    password_dialog(area);
    endwin();
    delscreen(screen);
    fclose(out);
    fclose(in);
    if (failures) { printf("%d failure(s)\n", failures); return 1; }
    printf("ok: the private chat dialogs say what they are for, take passwords unseen and show nothing of the chat\n");
    return 0;
}
