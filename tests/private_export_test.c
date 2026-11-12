/* Exporting a private chat asks for an unlock every time, even while it is
 * open, and writes nothing until it passes. */
#include "tui_harness.h"

#include <dirent.h>
#include <sys/stat.h>

static int failures;
#define CHECK(cond, what) do { if (!(cond)) { printf("FAIL: %s\n", what); failures++; } } while (0)

#define DIARY "27820000001@s.whatsapp.net"
#define MOM   "27820000002@s.whatsapp.net"

static int files_in(const char *dir) {
    DIR *d = opendir(dir);
    if (!d) return -1;
    int n = 0;
    struct dirent *e;
    while ((e = readdir(d))) if (e->d_name[0] != '.') n++;
    closedir(d);
    return n;
}

static void keys(Harness *h, const char *text) {
    for (const char *p = text; *p; p++) tui_app_private_key(h->app, 0, (unsigned char)*p);
}

int main(void) {
    Harness h;
    if (harness_open(&h, 0, "letmein") != 0) { printf("FAIL: the harness opens\n"); return 1; }
    char saves[128];
    snprintf(saves, sizeof(saves), "%s/saves", h.dir);
    mkdir(saves, 0700);
    Settings s = *settings_manager_current(h.settings);
    str_copy(s.download_dir, sizeof(s.download_dir), saves);
    settings_manager_apply(h.settings, &s);
    harness_chat(&h, 0, DIARY, "Diary", 1);
    harness_chat(&h, 0, MOM, "Mom", 0);

    tui_app_export_chat(h.app, MOM, 0);
    CHECK(files_in(saves) == 1, "a public chat is exported at once");

    tui_app_export_chat(h.app, DIARY, 0);
    CHECK(private_chat_guard_busy(&h.app->privacy) && files_in(saves) == 1, "a private one asks first and writes nothing");
    keys(&h, "wrong\r");
    CHECK(files_in(saves) == 1, "nor after a wrong password");
    keys(&h, "\x1b");
    CHECK(files_in(saves) == 1, "nor after Esc");

    tui_app_open_chat(h.app, DIARY);
    keys(&h, "letmein\r");
    CHECK(!strcmp(harness_open_jid(&h), DIARY), "opened and unlocked");
    tui_app_export_chat(h.app, DIARY, 0);
    CHECK(private_chat_guard_busy(&h.app->privacy) && files_in(saves) == 1, "even open and unlocked it asks again");
    keys(&h, "letmein\r");
    CHECK(!private_chat_guard_busy(&h.app->privacy) && files_in(saves) == 2, "and writes the file once it passes");

    harness_close(&h);
    if (failures) { printf("%d failure(s)\n", failures); return 1; }
    printf("ok: a private chat is exported only after an unlock, every time\n");
    return 0;
}
