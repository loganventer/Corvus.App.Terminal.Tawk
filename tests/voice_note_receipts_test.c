/* How a voice note or video you sent was received: the words the engine
 * builds (always the date and the exact time), and the conversation that
 * shows them under the note, opened and closed by a click on its head line. */
#include "clients/tui/message_view.h"
#include "clients/tui/tui_palette.h"
#include "engines/receipt_details_builder.h"
#include "utilities/clock_util.h"
#include "utilities/str_util.h"

#include <locale.h>
#include <ncurses.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int failures = 0;

#define CHECK(cond, what) do { if (!(cond)) { fprintf(stderr, "FAIL: %s\n", what); failures++; } } while (0)

#define MOM "27820000001@s.whatsapp.net"

#define SENT      1790000000
#define DELIVERED 1790000004
#define READ_AT   1790000090
#define PLAYED    1790000125
#define PLAYED_B  1790086525      /* the next day */

static void note(Message *m, const char *id, int from_me, MessageType type) {
    message_init(m);
    str_copy(m->id, sizeof(m->id), id);
    str_copy(m->chat_jid, sizeof(m->chat_jid), MOM);
    str_copy(m->sender_jid, sizeof(m->sender_jid), MOM);
    m->type = type;
    m->from_me = from_me;
    m->timestamp = SENT;
    m->duration_s = 52;
}

static Receipt receipt(const char *name, int64_t delivered, int64_t read, int64_t played) {
    Receipt r;
    memset(&r, 0, sizeof(r));
    str_copy(r.name, sizeof(r.name), name);
    str_copy(r.jid, sizeof(r.jid), MOM);
    r.delivered_at = delivered;
    r.read_at = read;
    r.played_at = played;
    return r;
}

static int has_line(const ReceiptDetails *d, const char *text) {
    for (int i = 0; i < d->line_count; i++) if (strstr(d->lines[i], text)) return 1;
    return 0;
}

static void test_the_words(void) {
    Message audio, text, theirs;
    note(&audio, "A1", 1, MESSAGE_TYPE_AUDIO);
    note(&text, "T1", 1, MESSAGE_TYPE_TEXT);
    note(&theirs, "A2", 0, MESSAGE_TYPE_AUDIO);
    Receipt played = receipt("Mom", DELIVERED, READ_AT, PLAYED);
    Receipt unplayed = receipt("Mom", DELIVERED, READ_AT, 0);
    ReceiptDetails d;
    char t[40], head[200];

    receipt_details_build(&audio, &played, 1, 0, &d);
    clock_format_exact(PLAYED, t, sizeof(t));
    snprintf(head, sizeof(head), "Played %s", t);
    CHECK(d.shown && d.played && strcmp(d.head, head) == 0, "played: the head line has the date and the exact time");
    CHECK(strlen(t) == 19 && t[4] == '-' && t[10] == ' ' && t[16] == ':', "to the second, with the date");
    CHECK(d.line_count == 4 && has_line(&d, "Sent ") && has_line(&d, "Delivered ") && has_line(&d, "Read ") && has_line(&d, "Played "),
          "and the lines are Sent, Delivered, Read and Played");

    receipt_details_build(&audio, &unplayed, 1, 0, &d);
    CHECK(d.shown && !d.played && strcmp(d.head, "Not played yet") == 0 && has_line(&d, "Played not yet"),
          "read but not played says so, and Played shows as not yet");

    receipt_details_build(&text, &played, 1, 0, &d);
    CHECK(!d.shown, "a text message has none");
    receipt_details_build(&theirs, &played, 1, 0, &d);
    CHECK(!d.shown, "nor a voice note someone sent you");
    receipt_details_build(&audio, NULL, 0, 0, &d);
    CHECK(!d.shown, "nor one that nobody has received yet");

    Message deleted = audio;
    deleted.deleted = 1;
    receipt_details_build(&deleted, &played, 1, 0, &d);
    CHECK(!d.shown, "nor one that was deleted");

    /* A group: the first play, how many of them, and a line each. */
    Receipt group[3] = { receipt("Anna", DELIVERED, READ_AT, PLAYED_B), receipt("Ben", DELIVERED, READ_AT, PLAYED),
                         receipt("Cara", DELIVERED, 0, 0) };
    receipt_details_build(&audio, group, 3, 1, &d);
    clock_format_exact(PLAYED, t, sizeof(t));
    snprintf(head, sizeof(head), "Played by 2 of 3, first %s", t);
    CHECK(d.shown && strcmp(d.head, head) == 0, "a group says how many played it and when the first did");
    CHECK(d.line_count == 4 && has_line(&d, "Anna: played ") && has_line(&d, "Ben: played ") && has_line(&d, "Cara: delivered "),
          "with a line for each person by how far they got");
    clock_format_exact(PLAYED_B, t, sizeof(t));
    CHECK(has_line(&d, t), "a play on another day carries its own date");

    message_dispose(&audio);
    message_dispose(&text);
    message_dispose(&theirs);
}

/* ---- the conversation ------------------------------------------------------ */

typedef struct Fake {
    Receipt receipts[2];
    int     count;
    int     asked;
} Fake;

static int fake_list(void *ctx, const Message *message, AccountId owner, Receipt *out, int max) {
    Fake *f = ctx;
    (void)owner;
    if (strcmp(message->id, "N1") != 0) return 0;
    f->asked++;
    int n = f->count < max ? f->count : max;
    for (int i = 0; i < n; i++) out[i] = f->receipts[i];
    return n;
}

static int on_screen(const char *words) {
    char line[512];
    for (int y = 0; y < LINES; y++) {
        if (mvinnstr(y, 0, line, sizeof(line) - 1) != ERR && strstr(line, words)) return 1;
    }
    return 0;
}

/* The row on screen that holds `words`, or -1. */
static int row_of(const char *words, int *column) {
    char line[512];
    for (int y = 0; y < LINES; y++) {
        if (mvinnstr(y, 0, line, sizeof(line) - 1) == ERR) continue;
        char *at = strstr(line, words);
        if (at) { *column = (int)(at - line); return y; }
    }
    return -1;
}

static void test_conversation(void) {
    setlocale(LC_ALL, "");
    FILE *out = fopen("/dev/null", "w"), *in = fopen("/dev/null", "r");
    SCREEN *screen = out && in ? newterm("xterm-256color", out, in) : NULL;
    CHECK(screen != NULL, "a screen to draw on");
    if (!screen) return;
    tui_palette_init();
    resizeterm(40, 100);

    Message msgs[2];
    note(&msgs[0], "N1", 1, MESSAGE_TYPE_AUDIO);
    note(&msgs[1], "N2", 1, MESSAGE_TYPE_AUDIO);
    Fake fake = { { receipt("Mom", DELIVERED, READ_AT, PLAYED) }, 1, 0 };
    ReceiptSource source = { &fake, fake_list };
    char played[40], when[64];
    clock_format_exact(PLAYED, played, sizeof(played));
    snprintf(when, sizeof(when), "Played %s", played);

    MessageViewContext ctx;
    memset(&ctx, 0, sizeof(ctx));
    ctx.title = "Mom";
    ctx.playing_path = "";
    UiRect rect = { 0, 0, 40, 100 };
    MessageView view;
    message_view_init(&view);

    erase();
    message_view_render(&view, rect, msgs, 2, &ctx);
    int rows_without = view.row_count;
    CHECK(!on_screen("Played") && !message_view_has_details(&view, 0), "with no source, nothing is shown");

    erase();
    ctx.receipts = &source;
    message_view_render(&view, rect, msgs, 2, &ctx);
    CHECK(on_screen(when) && message_view_has_details(&view, 0) && !message_view_has_details(&view, 1),
          "a voice note with receipts shows its head line, with the date and exact time, and one without shows none");
    CHECK(!on_screen("Delivered") && view.row_count == rows_without + 1, "closed, it takes one row");

    int column = 0;
    int y = row_of("Played", &column);
    CHECK(y >= 0 && message_view_hit_details(&view, y, column + 2) == 0, "a click on the head line is a click on its details");
    CHECK(message_view_hit_details(&view, y - 1, column + 2) != 0, "and one above it is not");

    erase();
    message_view_toggle_details(&view, "N1");
    message_view_render(&view, rect, msgs, 2, &ctx);
    CHECK(message_view_details_open(&view, "N1") && on_screen("Sent ") && on_screen("Delivered ") && on_screen("Read ") && on_screen(played),
          "opened, it shows Sent, Delivered, Read and Played with the exact times");
    CHECK(view.row_count == rows_without + 1 + 4, "which is four more rows");

    erase();
    message_view_toggle_details(&view, "N1");
    message_view_render(&view, rect, msgs, 2, &ctx);
    CHECK(!message_view_details_open(&view, "N1") && !on_screen("Delivered "), "and closes again");

    erase();
    fake.count = 0;
    message_view_render(&view, rect, msgs, 2, &ctx);
    CHECK(!on_screen("Played") && !message_view_has_details(&view, 0), "a note nobody has received yet shows nothing");

    erase();
    fake.count = 1;
    ctx.veiled = 1;
    message_view_render(&view, rect, msgs, 2, &ctx);
    CHECK(!on_screen("Played") && !on_screen("Delivered"), "a soft-locked chat shows none of it");

    message_view_dispose(&view);
    message_dispose(&msgs[0]);
    message_dispose(&msgs[1]);
    endwin();
    delscreen(screen);
    fclose(out);
    fclose(in);
}

int main(void) {
    test_the_words();
    test_conversation();
    if (failures) { fprintf(stderr, "%d check(s) failed\n", failures); return 1; }
    printf("voice note receipts: all checks passed\n");
    return 0;
}
