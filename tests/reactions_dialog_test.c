/* Who reacted: the order the list shows people in, the dialog that lists
 * them, and finding a click on the reactions chip of a message. */
#include "clients/tui/message_view.h"
#include "clients/tui/reactions_dialog.h"
#include "clients/tui/tui_palette.h"
#include "engines/reaction_grouping.h"
#include "utilities/str_util.h"

#include <locale.h>
#include <ncurses.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int failures = 0;

#define CHECK(cond, what) do { if (!(cond)) { fprintf(stderr, "FAIL: %s\n", what); failures++; } } while (0)

#define THUMB "\xF0\x9F\x91\x8D"
#define HEART "\xE2\x9D\xA4"
#define LAUGH "\xF0\x9F\x98\x82"

static Reaction by(const char *name, const char *emoji) {
    Reaction r;
    memset(&r, 0, sizeof(r));
    str_copy(r.name, sizeof(r.name), name);
    str_copy(r.sender, sizeof(r.sender), name);
    str_copy(r.emoji, sizeof(r.emoji), emoji);
    return r;
}

static void test_grouping(void) {
    Reaction r[5] = { by("Anna", HEART), by("Ben", THUMB), by("Cara", HEART), by("Dan", THUMB), by("Eve", THUMB) };
    reaction_grouping_sort(r, 5);
    CHECK(strcmp(r[0].emoji, THUMB) == 0 && strcmp(r[1].emoji, THUMB) == 0 && strcmp(r[2].emoji, THUMB) == 0,
          "the emoji most people used comes first");
    CHECK(strcmp(r[0].name, "Ben") == 0 && strcmp(r[1].name, "Dan") == 0 && strcmp(r[2].name, "Eve") == 0 &&
          strcmp(r[3].name, "Anna") == 0 && strcmp(r[4].name, "Cara") == 0, "people who share an emoji keep their order");

    Reaction tie[4] = { by("Anna", HEART), by("Ben", THUMB), by("Cara", THUMB), by("Dan", HEART) };
    reaction_grouping_sort(tie, 4);
    CHECK(strcmp(tie[0].emoji, HEART) == 0 && strcmp(tie[2].emoji, THUMB) == 0, "equal groups keep the order they first appeared in");

    ReactionGroup g[8];
    int n = reaction_grouping_groups(r, 5, g, 8);
    CHECK(n == 2 && strcmp(g[0].emoji, THUMB) == 0 && g[0].count == 3 && strcmp(g[1].emoji, HEART) == 0 && g[1].count == 2,
          "the groups say how many used each emoji");
    CHECK(reaction_grouping_groups(r, 0, g, 8) == 0, "no reactions, no groups");
    CHECK(reaction_grouping_groups(r, 5, g, 1) == 1, "the groups stop at the room given");
    reaction_grouping_sort(r, 0);
}

static int on_screen(const char *words) {
    char line[512];
    for (int y = 0; y < LINES; y++) {
        if (mvinnstr(y, 0, line, sizeof(line) - 1) != ERR && strstr(line, words)) return 1;
    }
    return 0;
}

static void test_dialog(void) {
    Reaction r[3] = { by("Anna", HEART), by("Ben", THUMB), by("Cara", THUMB) };
    UiRect area = { 0, 0, 40, 100 };
    ReactionsDialog d;
    reactions_dialog_open(&d, "M1");
    CHECK(d.open && strcmp(d.message_id, "M1") == 0, "it opens on the message");

    erase();
    reactions_dialog_render(&d, area, r, 3);
    CHECK(on_screen("Reactions 3") && on_screen(THUMB " 2") && on_screen(HEART " 1"), "the title counts the reactions and each emoji");
    CHECK(on_screen("Anna") && on_screen("Ben") && on_screen("Cara"), "each person is listed");

    char first[512], last[512];
    int ben = -1, anna = -1;
    for (int y = 0; y < LINES; y++) {
        if (mvinnstr(y, 0, first, sizeof(first) - 1) == ERR) continue;
        if (strstr(first, "Ben")) ben = y;
        if (strstr(first, "Anna")) anna = y;
    }
    (void)last;
    CHECK(ben >= 0 && anna > ben, "the emoji most used is listed first");

    reactions_dialog_key(&d, 1, KEY_DOWN);
    CHECK(d.selected == 1, "the arrow keys move through the list");
    reactions_dialog_wheel(&d, 5);
    CHECK(d.selected == 2, "the wheel stops at the last person");
    CHECK(reactions_dialog_click(&d, d.last_rect.y + 1, d.last_rect.x + 2) == POPUP_NONE && d.open, "a click inside leaves it open");
    CHECK(reactions_dialog_click(&d, 0, 0) == POPUP_CLOSED && !d.open, "a click outside closes it");

    reactions_dialog_open(&d, "M1");
    CHECK(reactions_dialog_key(&d, 0, 27) == POPUP_CLOSED && !d.open, "Esc closes it");

    reactions_dialog_open(&d, "M1");
    erase();
    reactions_dialog_render(&d, area, r, 0);
    CHECK(on_screen("No reactions."), "with none it says so");
}

static void text_message(Message *m, const char *id, int from_me, const char *text, const char *reactions) {
    message_init(m);
    str_copy(m->id, sizeof(m->id), id);
    str_copy(m->chat_jid, sizeof(m->chat_jid), "120363000000000001@g.us");
    m->from_me = from_me;
    m->type = MESSAGE_TYPE_TEXT;
    m->timestamp = 1790000000;
    message_set_text(m, text);
    str_copy(m->sender_name, sizeof(m->sender_name), "Anna");
    str_copy(m->reactions, sizeof(m->reactions), reactions);
}

/* The row and column of `words` on the screen. The line comes back as UTF-8, so the column counts characters, not bytes. */
static int find(const char *words, int *y, int *x) {
    char line[512];
    for (int row = 0; row < LINES; row++) {
        if (mvinnstr(row, 0, line, sizeof(line) - 1) == ERR) continue;
        char *at = strstr(line, words);
        if (!at) continue;
        int column = 0;
        for (const char *c = line; c < at; c++) column += ((unsigned char)*c & 0xC0) != 0x80;
        *y = row;
        *x = column;
        return 1;
    }
    return 0;
}

static void test_chip_hit(void) {
    setlocale(LC_ALL, "");
    FILE *out = fopen("/dev/null", "w"), *in = fopen("/dev/null", "r");
    SCREEN *screen = out && in ? newterm("xterm-256color", out, in) : NULL;
    CHECK(screen != NULL, "a screen to draw on");
    if (!screen) return;
    tui_palette_init();
    resizeterm(40, 100);

    Message msgs[2];
    text_message(&msgs[0], "A1", 0, "Lunch on Friday?", "x 2");
    text_message(&msgs[1], "B1", 1, "Count me in", "");
    MessageViewContext ctx;
    memset(&ctx, 0, sizeof(ctx));
    ctx.title = "Family";
    ctx.is_group = 1;
    ctx.playing_path = "";
    UiRect rect = { 0, 0, 40, 100 };
    MessageView view;
    message_view_init(&view);

    erase();
    message_view_render(&view, rect, msgs, 2, &ctx);
    int y = 0, x = 0;
    CHECK(find("x 2", &y, &x), "the reactions show on the bubble");
    CHECK(message_view_hit_reactions(&view, y, x) == 0, "a click on the chip finds its message");
    CHECK(message_view_hit_reactions(&view, y, x + 2) == 0, "and on its last column");
    CHECK(message_view_hit_reactions(&view, y, x - 3) == -1, "a click beside the chip does not");
    int text_y = 0, text_x = 0;
    CHECK(find("Lunch on Friday?", &text_y, &text_x) && message_view_hit_reactions(&view, text_y, text_x) == -1,
          "a click on the text is not a click on the reactions");
    CHECK(find("Count me in", &text_y, &text_x) && message_view_hit_reactions(&view, text_y, text_x) == -1,
          "a message without reactions has no chip");

    ctx.veiled = 1;
    erase();
    message_view_render(&view, rect, msgs, 2, &ctx);
    CHECK(message_view_hit_reactions(&view, y, x) == -1, "a locked chat has no chip to click");

    message_view_dispose(&view);
    message_dispose(&msgs[0]);
    message_dispose(&msgs[1]);
    endwin();
    delscreen(screen);
    fclose(out);
    fclose(in);
}

static void test_dialog_screen(void) {
    setlocale(LC_ALL, "");
    FILE *out = fopen("/dev/null", "w"), *in = fopen("/dev/null", "r");
    SCREEN *screen = out && in ? newterm("xterm-256color", out, in) : NULL;
    CHECK(screen != NULL, "a screen to draw on");
    if (!screen) return;
    tui_palette_init();
    resizeterm(40, 100);
    test_dialog();
    endwin();
    delscreen(screen);
    fclose(out);
    fclose(in);
}

int main(void) {
    test_grouping();
    test_dialog_screen();
    test_chip_hit();
    if (failures) return 1;
    printf("reactions_dialog_test: ok\n");
    return 0;
}
