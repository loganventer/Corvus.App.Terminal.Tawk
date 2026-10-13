/* Message info: receipt events from the backends, the receipt store and the panel. */
#include "clients/tui/message_info_panel.h"
#include "clients/tui/tui_palette.h"
#include "core/event.h"
#include "engines/receipt_elapsed_text.h"
#include "resource_access/json_protocol.h"
#include "resource_access/sqlite_database.h"
#include "resource_access/sqlite_receipt_store.h"
#include "utilities/clock_util.h"
#include "utilities/log.h"
#include "utilities/str_util.h"

#include <locale.h>
#include <ncurses.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

static int failures = 0;

#define CHECK(cond, what) do { if (!(cond)) { fprintf(stderr, "FAIL: %s\n", what); failures++; } } while (0)

static void test_protocol(void) {
    Event e;
    event_init(&e, EVENT_NONE);
    CHECK(json_protocol_decode("{\"evt\":\"receipt\",\"id\":\"ABC\",\"by\":\"27821234567@s.whatsapp.net\",\"kind\":\"read\",\"at\":1790000000}", &e) == 0 &&
          e.type == EVENT_MESSAGE_RECEIPT && strcmp(e.id, "ABC") == 0 && strcmp(e.jid, "27821234567@s.whatsapp.net") == 0 &&
          e.receipt == RECEIPT_READ && e.at == 1790000000, "a receipt event is decoded");
    event_dispose(&e);
    event_init(&e, EVENT_NONE);
    CHECK(json_protocol_decode("{\"evt\":\"receipt\",\"id\":\"ABC\",\"by\":\"x@s.whatsapp.net\",\"kind\":\"seen\",\"at\":1}", &e) != 0,
          "an unknown kind is refused");
    event_dispose(&e);
}

static void test_store(void) {
    char path[] = "/tmp/tawk-receipts-test-XXXXXX";
    int fd = mkstemp(path);
    if (fd < 0) { perror("mkstemp"); failures++; return; }
    close(fd);
    unlink(path);
    /* tawk logs to a file only; show it when the database will not open. */
    char log_path[] = "/tmp/tawk-receipts-log-XXXXXX";
    int log_fd = mkstemp(log_path);
    if (log_fd >= 0) close(log_fd);
    log_open(log_path, LOG_LEVEL_DEBUG);
    sqlite3 *db = sqlite_database_open(path, NULL);
    log_close();
    CHECK(db != NULL, "the database opens and migrates");
    if (!db) {
        fprintf(stderr, "sqlite %s; tawk's log:\n", sqlite3_libversion());
        FILE *f = fopen(log_path, "r");
        char line[512];
        while (f && fgets(line, sizeof(line), f)) fputs(line, stderr);
        if (f) fclose(f);
    }
    unlink(log_path);
    if (!db) return;
    IReceiptStore *s = sqlite_receipt_store_create(db, ACCOUNT_ID_FIRST);
    Receipt r[8];

    s->put(s, "M1", "ann", RECEIPT_DELIVERED, 100);
    s->put(s, "M1", "bob", RECEIPT_DELIVERED, 110);
    s->put(s, "M1", "bob", RECEIPT_READ, 150);
    s->put(s, "M1", "ann", RECEIPT_DELIVERED, 90);      /* an earlier duplicate wins */
    s->put(s, "M1", "cat", RECEIPT_PLAYED, 200);        /* played implies read and delivered */
    s->put(s, "M1", "bob", RECEIPT_READ, 300);          /* a later read keeps the first */
    s->put(s, "M2", "ann", RECEIPT_READ, 500);
    int n = s->list(s, "M1", r, 8);
    CHECK(n == 3, "three people have receipts for M1");
    CHECK(n == 3 && strcmp(r[0].jid, "bob") == 0 && r[0].read_at == 150 && r[0].delivered_at == 110,
          "the first reader comes first, with the first read time");
    CHECK(n == 3 && strcmp(r[1].jid, "cat") == 0 && r[1].read_at == 200 && r[1].played_at == 200 && r[1].delivered_at == 200,
          "played counts as read and delivered");
    CHECK(n == 3 && strcmp(r[2].jid, "ann") == 0 && r[2].delivered_at == 90 && r[2].read_at == 0,
          "delivered only comes last, with the earliest time");
    CHECK(s->put(s, "M1", "dan", RECEIPT_NONE, 1) != 0 && s->put(s, "M1", "dan", RECEIPT_READ, 0) != 0,
          "empty receipts are refused");

    s->reassign_jid(s, "ann", "ann-phone");
    n = s->list(s, "M2", r, 8);
    CHECK(n == 1 && strcmp(r[0].jid, "ann-phone") == 0, "receipts follow an alias merge");

    s->destroy(s);
    sqlite_database_close(db);
    unlink(path);
}

static void test_panel(void) {
    Message m;
    memset(&m, 0, sizeof(m));
    snprintf(m.id, sizeof(m.id), "M1");
    snprintf(m.chat_jid, sizeof(m.chat_jid), "123@g.us");
    char text[] = "Hello group\nsecond line";
    m.text = text;
    m.timestamp = 100;
    MessageInfoPanel p;
    message_info_panel_open(&p, &m, 1);
    CHECK(p.open && p.group && strcmp(p.message_id, "M1") == 0 && strcmp(p.excerpt, "Hello group") == 0,
          "the panel opens on the message, showing its first line");
    message_info_panel_key(&p, 1, KEY_DOWN);
    CHECK(p.open && p.scroll == 1, "arrows scroll");
    CHECK(message_info_panel_key(&p, 0, 27) == POPUP_CLOSED && !p.open, "Esc closes");
}

/* ---- the openable lines ---------------------------------------------------- */

#define SENT_AT   1790000000
#define DELIVERED 1790000004
#define READ_AT   1790000090
#define PLAYED_AT 1790010845          /* 3 h 0 min 45 s after it was sent */

static void test_elapsed(void) {
    char s[96];
    receipt_elapsed_text(SENT_AT, SENT_AT + 41, s, sizeof(s));
    CHECK(strcmp(s, "41 s after it was sent") == 0, "seconds");
    receipt_elapsed_text(SENT_AT, SENT_AT + 252, s, sizeof(s));
    CHECK(strcmp(s, "4 min 12 s after it was sent") == 0, "minutes and seconds");
    receipt_elapsed_text(SENT_AT, PLAYED_AT, s, sizeof(s));
    CHECK(strcmp(s, "3 h 0 min after it was sent") == 0, "hours and minutes");
    receipt_elapsed_text(SENT_AT, SENT_AT + 3 * 86400 + 2 * 3600 + 5, s, sizeof(s));
    CHECK(strcmp(s, "3 d 2 h after it was sent") == 0, "days and hours");
    receipt_elapsed_text(SENT_AT, SENT_AT, s, sizeof(s));
    CHECK(s[0] == '\0', "nothing for the same moment");
    receipt_elapsed_text(SENT_AT, SENT_AT - 5, s, sizeof(s));
    CHECK(s[0] == '\0', "nor for a clock that is behind");
    receipt_elapsed_text(0, SENT_AT, s, sizeof(s));
    CHECK(s[0] == '\0', "nor without a send time");
}

static int on_screen(const char *words) {
    char line[512];
    for (int y = 0; y < LINES; y++) {
        if (mvinnstr(y, 0, line, sizeof(line) - 1) != ERR && strstr(line, words)) return 1;
    }
    return 0;
}

static int row_of(const char *words, int *column) {
    char line[512];
    for (int y = 0; y < LINES; y++) {
        if (mvinnstr(y, 0, line, sizeof(line) - 1) == ERR) continue;
        char *at = strstr(line, words);
        if (at) { *column = (int)(at - line); return y; }
    }
    return -1;
}

static Receipt receipt_of(const char *name, int64_t delivered, int64_t read, int64_t played) {
    Receipt r;
    memset(&r, 0, sizeof(r));
    str_copy(r.name, sizeof(r.name), name);
    str_copy(r.jid, sizeof(r.jid), name);
    r.delivered_at = delivered;
    r.read_at = read;
    r.played_at = played;
    return r;
}

static void test_openable_lines(void) {
    setlocale(LC_ALL, "");
    FILE *out = fopen("/dev/null", "w"), *in = fopen("/dev/null", "r");
    SCREEN *screen = out && in ? newterm("xterm-256color", out, in) : NULL;
    CHECK(screen != NULL, "a screen to draw on");
    if (!screen) return;
    tui_palette_init();
    resizeterm(40, 100);
    UiRect area = { 0, 0, 40, 100 };
    char read_exact[40], played_exact[40], sent_exact[40];
    clock_format_exact(READ_AT, read_exact, sizeof(read_exact));
    clock_format_exact(PLAYED_AT, played_exact, sizeof(played_exact));
    clock_format_exact(SENT_AT, sent_exact, sizeof(sent_exact));

    /* A voice note you sent, to one person. */
    Message m;
    message_init(&m);
    str_copy(m.id, sizeof(m.id), "V1");
    str_copy(m.chat_jid, sizeof(m.chat_jid), "27820000001@s.whatsapp.net");
    m.from_me = 1;
    m.type = MESSAGE_TYPE_AUDIO;
    m.timestamp = SENT_AT;
    Receipt one = receipt_of("Mom", DELIVERED, READ_AT, PLAYED_AT);
    MessageInfoPanel p;
    message_info_panel_open(&p, &m, 0);

    erase();
    message_info_panel_render(&p, area, &one, 1, 1);
    CHECK(on_screen("Sent") && on_screen("Delivered") && on_screen("Read") && on_screen("Played") && on_screen("\xE2\x96\xB8"),
          "the lines show with an arrow that opens each");
    CHECK(p.item_count == 4 && !on_screen(played_exact) && !on_screen("after it was sent"), "closed, no exact time shows");

    /* Down to Played (Sent, Delivered, Read, Played), then open it with Enter. */
    message_info_panel_key(&p, 1, KEY_DOWN);
    message_info_panel_key(&p, 1, KEY_DOWN);
    message_info_panel_key(&p, 1, KEY_DOWN);
    CHECK(p.open && p.cursor == 3, "the arrow keys move between the lines");
    CHECK(message_info_panel_key(&p, 0, '\n') == POPUP_NONE && p.open, "Enter opens the line and does not close the dialog");
    erase();
    message_info_panel_render(&p, area, &one, 1, 1);
    CHECK(on_screen(played_exact) && on_screen("3 h 0 min after it was sent") && on_screen("\xE2\x96\xBE"),
          "opened, a line gives the exact date and time and how long after the message it came");
    CHECK(!on_screen(read_exact), "and only that line opens");

    /* A click on a line opens it too. */
    int column = 0;
    int y = row_of("Read", &column);
    CHECK(y >= 0 && message_info_panel_click(&p, y, column + 2) == POPUP_NONE && p.open, "a click on a line does not close the dialog");
    erase();
    message_info_panel_render(&p, area, &one, 1, 1);
    CHECK(on_screen(read_exact), "it opens that line");

    /* Left closes the line the arrow keys are on; Esc closes the dialog. */
    message_info_panel_key(&p, 1, KEY_LEFT);
    erase();
    message_info_panel_render(&p, area, &one, 1, 1);
    CHECK(!on_screen(read_exact) && on_screen(played_exact), "Left closes the line it is on");
    CHECK(message_info_panel_click(&p, 0, 0) == POPUP_CLOSED && !p.open, "a click outside closes the dialog");

    /* Not played yet: the line is there, says so, and has nothing to open. */
    Receipt unplayed = receipt_of("Mom", DELIVERED, READ_AT, 0);
    message_info_panel_open(&p, &m, 0);
    erase();
    message_info_panel_render(&p, area, &unplayed, 1, 1);
    CHECK(on_screen("Played") && on_screen("not yet") && p.item_count == 3, "a note not played yet shows Played as not yet, with no arrow to open");

    /* A text message you sent has no Played line unless it was played. */
    Message text = m;
    text.type = MESSAGE_TYPE_TEXT;
    message_info_panel_open(&p, &text, 0);
    erase();
    message_info_panel_render(&p, area, &unplayed, 1, 1);
    CHECK(!on_screen("Played"), "a text message shows no Played line");

    /* A group: a line a person, opening to each step they reached. */
    Receipt group[2] = { receipt_of("Anna", DELIVERED, READ_AT, PLAYED_AT), receipt_of("Ben", DELIVERED, 0, 0) };
    message_info_panel_open(&p, &m, 1);
    erase();
    message_info_panel_render(&p, area, group, 2, 1);
    CHECK(on_screen("Read by 1") && on_screen("Anna") && on_screen("Delivered to 1") && on_screen("Ben"), "a group lists who read it and who got it");
    message_info_panel_key(&p, 1, KEY_DOWN);                  /* Sent, then Anna */
    message_info_panel_key(&p, 0, ' ');
    erase();
    message_info_panel_render(&p, area, group, 2, 1);
    CHECK(on_screen(played_exact) && on_screen(read_exact) && on_screen("Delivered 20"), "a person opens to the exact time of each step");

    message_dispose(&m);
    endwin();
    delscreen(screen);
    fclose(out);
    fclose(in);
}

int main(void) {
    test_protocol();
    test_store();
    test_panel();
    test_elapsed();
    test_openable_lines();
    if (failures) return 1;
    printf("ok: message info shows who received and read a message\n");
    return 0;
}
