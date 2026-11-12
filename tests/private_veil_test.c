/* What a private chat shows: veiled while locked (the conversation, its
 * subtitle and, with the messages, its transcripts and TL;DR), as it is
 * once unlocked, and veiled again when it locks. */
#include "clients/tui/chat_subtitle.h"
#include "clients/tui/contact_panel.h"
#include "clients/tui/tui_palette.h"
#include "core/contact_profile.h"

#include <locale.h>
#include <ncurses.h>
#include "tui_harness.h"

static int failures;
#define CHECK(cond, what) do { if (!(cond)) { printf("FAIL: %s\n", what); failures++; } } while (0)

#define DIARY "27820000001@s.whatsapp.net"
#define MOM   "27820000002@s.whatsapp.net"

static void save(Harness *h, const char *id, const char *jid, const char *text, int64_t ts) {
    Message m;
    message_init(&m);
    str_copy(m.id, sizeof(m.id), id);
    str_copy(m.chat_jid, sizeof(m.chat_jid), jid);
    m.timestamp = ts;
    message_set_text(&m, text);
    h->accounts[0].messages->save(h->accounts[0].messages, &m);
    message_dispose(&m);
}

static int search_finds(Harness *h, const char *query, const char *id) {
    Message *hits = NULL;
    int n = 0;
    tui_app_private_search(h->app, query, 50, &hits, &n);
    int yes = 0;
    for (int i = 0; i < n; i++) if (!strcmp(hits[i].id, id)) yes = 1;
    message_array_free(hits, n);
    return yes;
}

static int pane_veiled(Harness *h) {
    return tui_app_private_veiled(h->app, messaging_manager_open_chat_info(h->app->deps.messaging));
}

static int on_screen(const char *words) {
    char line[512];
    for (int row = 0; row < LINES; row++) {
        if (mvinnstr(row, 0, line, sizeof(line) - 1) == ERR) continue;
        if (strstr(line, words)) return 1;
    }
    return 0;
}

/* The contact card of a locked private chat shows its name and nothing of its person. */
static void contact_card(void) {
    setlocale(LC_ALL, "");
    FILE *out = fopen("/dev/null", "w"), *in = fopen("/dev/null", "r");
    SCREEN *screen = out && in ? newterm("xterm-256color", out, in) : NULL;
    if (!screen) { printf("FAIL: a screen to draw on\n"); failures++; return; }
    tui_palette_init();
    resizeterm(40, 100);
    Chat c;
    chat_init(&c, DIARY);
    str_copy(c.name, sizeof(c.name), "Diary");
    c.is_private = 1;
    ContactProfile pr;
    contact_profile_init(&pr, DIARY);
    str_copy(pr.about, sizeof(pr.about), "Secret about words");
    ContactPanel panel;
    memset(&panel, 0, sizeof(panel));
    contact_panel_open(&panel, &c, 0);
    UiRect area = { 0, 0, 40, 100 };
    panel.veiled = 1;
    erase();
    contact_panel_render(&panel, area, &c, &pr, NULL, NULL, NULL, NULL, 0, NULL);
    CHECK(on_screen("Diary") && !on_screen("Secret about words"), "a locked private chat's card shows no about text");
    panel.veiled = 0;
    erase();
    contact_panel_render(&panel, area, &c, &pr, NULL, NULL, NULL, NULL, 0, NULL);
    CHECK(on_screen("Secret about words"), "an unlocked one's does");
    contact_profile_dispose(&pr);
    endwin();
    delscreen(screen);
    fclose(out);
    fclose(in);
}

int main(void) {
    contact_card();
    Harness h;
    if (harness_open(&h, 0, "letmein") != 0) { printf("FAIL: the harness opens\n"); return 1; }
    harness_chat(&h, 0, DIARY, "Diary", 0);
    harness_chat(&h, 0, MOM, "Mom", 0);

    tui_app_open_chat(h.app, DIARY);
    CHECK(!pane_veiled(&h), "a public chat's pane is not veiled");
    messaging_manager_set_private(h.app->deps.messaging, DIARY, 1);     /* made private while it is open */
    CHECK(pane_veiled(&h), "a locked private chat's pane is veiled");
    const Chat *open = messaging_manager_open_chat_info(h.app->deps.messaging);
    char out[96] = "x";
    chat_subtitle_text(settings_manager_current(h.settings), open, pane_veiled(&h), "about text", 0, out, sizeof(out));
    CHECK(out[0] == '\0', "and its subtitle says nothing");

    tui_app_open_chat(h.app, MOM);
    tui_app_open_chat(h.app, DIARY);
    harness_type(&h, "letmein");
    CHECK(!pane_veiled(&h), "once unlocked the pane is clear");
    open = messaging_manager_open_chat_info(h.app->deps.messaging);
    chat_subtitle_text(settings_manager_current(h.settings), open, pane_veiled(&h), "about text", 0, out, sizeof(out));
    CHECK(!strcmp(out, "about text"), "and its subtitle shows again");

    save(&h, "D1", DIARY, "the braai was lovely", 200);
    save(&h, "M1", MOM, "braai on Sunday?", 100);
    CHECK(search_finds(&h, "braai", "D1") && search_finds(&h, "braai", "M1"), "tawk's search lists an unlocked private chat's messages");

    tui_app_private_relock(h.app);
    CHECK(pane_veiled(&h), "locking it again veils it again");
    CHECK(!search_finds(&h, "braai", "D1") && search_finds(&h, "braai", "M1"), "and a locked one's are left out of search");
    tui_app_show_portrait(h.app, DIARY);
    CHECK(!h.app->viewer.open, "and its picture cannot be opened full size");

    harness_close(&h);
    if (failures) { printf("%d failure(s)\n", failures); return 1; }
    printf("ok: a private chat is veiled while locked, shown once unlocked, and veiled again when it locks\n");
    return 0;
}
