/* The header's + and name, the profile dialogs and the status composer:
 * what each key or click asks the app to do. */
#include "clients/tui/header_bar.h"
#include "clients/tui/mac_option_keys.h"
#include "clients/tui/tui_key_newline.h"
#include "clients/tui/profile_dialogs.h"
#include "clients/tui/splash_view.h"
#include "clients/tui/status_feed_dialogs.h"
#include "clients/tui/status_composer_dialog.h"
#include "clients/tui/tui_palette.h"

#include <locale.h>
#include <ncurses.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int failures = 0;

#define CHECK(cond, what) do { if (!(cond)) { fprintf(stderr, "FAIL: %s\n", what); failures++; } } while (0)

static void type(ProfileDialogs *d, const char *text) {
    for (const char *c = text; *c; c++) profile_dialogs_key(d, 0, (unsigned char)*c, 1, 1);
}

static void test_header(void) {
    UiRect bar = { 0, 0, 1, 100 };
    HeaderModel m = { .user_name = "Logan", .status = "o", .show_post = 1 };
    HeaderHits hits;
    header_bar_render(bar, &m, &hits);
    CHECK(hits.profile.w == 5 && hits.profile.x == 100 - 3 - 7, "the name is the profile target, right of the status");
    CHECK(header_bar_hit_profile(&hits, 0, hits.profile.x) && !header_bar_hit_profile(&hits, 0, hits.profile.x - 2),
          "the status emoji does not open the profile");
    CHECK(hits.post.w == 3 && hits.post.x + hits.post.w < hits.profile.x, "the + is left of the name");
    char clock_cell = (char)(mvinch(0, hits.post.x + hits.post.w) & A_CHARTEXT);
    CHECK(clock_cell >= '0' && clock_cell <= '9', "the clock follows the + directly");
    CHECK(header_bar_hit_post(&hits, 0, hits.post.x + 1), "clicking + posts a status");
    m.show_post = 0;
    m.user_name = "";
    header_bar_render(bar, &m, &hits);
    CHECK(!header_bar_hit_post(&hits, 0, 60) && hits.post.w == 0 && hits.profile.w == 0,
          "while linking there is no + and no name to click");
}

static void test_profile_dialogs(void) {
    ProfileDialogs d;
    profile_dialogs_open(&d);
    CHECK(profile_dialogs_is_open(&d) && !profile_dialogs_editing(&d), "the profile opens on the view");
    CHECK(profile_dialogs_key(&d, 0, '\n', 1, 1) == PROFILE_REQUEST_EDIT_TEXT && profile_dialogs_field(&d) == PROFILE_FIELD_NAME,
          "Enter on Name asks to edit the name");
    profile_dialogs_edit_text(&d, "Old", 25);
    CHECK(profile_dialogs_editing(&d), "the editor opens");
    profile_dialogs_key(&d, 1, KEY_BACKSPACE, 1, 1);
    profile_dialogs_key(&d, 1, KEY_BACKSPACE, 1, 1);
    profile_dialogs_key(&d, 1, KEY_BACKSPACE, 1, 1);
    type(&d, "New name");
    CHECK(profile_dialogs_key(&d, 0, '\r', 1, 1) == PROFILE_REQUEST_SAVE_TEXT, "Enter saves");
    char *text = profile_dialogs_text(&d);
    CHECK(text && strcmp(text, "New name") == 0, "the edited text is handed over");
    free(text);
    profile_dialogs_text_refused(&d, "Too long");
    CHECK(profile_dialogs_editing(&d) && strcmp(d.text.error, "Too long") == 0, "a refusal keeps the editor open with the reason");
    type(&d, "x");
    CHECK(d.text.error[0] == '\0', "typing clears the reason");
    profile_dialogs_text_saved(&d);
    CHECK(profile_dialogs_is_open(&d) && !profile_dialogs_editing(&d), "once saved it goes back to the view");

    profile_dialogs_key(&d, 1, KEY_DOWN, 1, 1);
    CHECK(profile_dialogs_key(&d, 0, '\n', 1, 1) == PROFILE_REQUEST_EDIT_TEXT && profile_dialogs_field(&d) == PROFILE_FIELD_ABOUT,
          "Enter on About asks to edit the about text");
    profile_dialogs_edit_text(&d, "", 139);
    for (int i = 0; i < 200; i++) profile_dialogs_key(&d, 0, 'a', 1, 1);
    CHECK(text_field_length(&d.text.input) == 139, "the about text stops at its limit");
    CHECK(profile_dialogs_key(&d, 0, 27, 1, 1) == PROFILE_REQUEST_REDRAW && !profile_dialogs_editing(&d) && profile_dialogs_is_open(&d),
          "Esc in the editor goes back to the view");

    profile_dialogs_key(&d, 1, KEY_DOWN, 1, 1);
    CHECK(profile_dialogs_key(&d, 0, '\n', 1, 0) == PROFILE_REQUEST_REDRAW && d.photo.open, "Enter on Photo opens the photo menu");
    profile_dialogs_key(&d, 1, KEY_DOWN, 1, 0);
    profile_dialogs_key(&d, 1, KEY_DOWN, 1, 0);
    profile_dialogs_key(&d, 1, KEY_DOWN, 1, 0);
    CHECK(profile_dialogs_photo_choice(&d) == PROFILE_PHOTO_PASTE, "without a photo, View and Remove are skipped");
    CHECK(profile_dialogs_key(&d, 0, '\n', 1, 0) == PROFILE_REQUEST_PHOTO && profile_dialogs_photo_choice(&d) == PROFILE_PHOTO_PASTE,
          "choosing a photo action asks the app to do it");
    CHECK(profile_dialogs_key(&d, 0, 27, 1, 1) == PROFILE_REQUEST_CLOSED && !profile_dialogs_is_open(&d), "Esc on the view closes it");
}

static void test_status_composer(void) {
    StatusComposerDialog d;
    status_composer_dialog_open(&d, 700);
    CHECK(d.kind == STATUS_KIND_TEXT, "a new status starts as text");
    CHECK(status_composer_dialog_key(&d, 0, 0x0F) == STATUS_REQUEST_NONE, "text statuses have no file to choose");
    status_composer_dialog_key(&d, 0, 0x02);
    CHECK(d.background == 1, "Ctrl+B changes the background");
    status_composer_dialog_key(&d, 0, 'h');
    status_composer_dialog_key(&d, 0, 'i');
    CHECK(status_composer_dialog_key(&d, 0, '\n') == STATUS_REQUEST_POST, "Enter posts");
    char *text = status_composer_dialog_text(&d);
    CHECK(text && strcmp(text, "hi") == 0, "the words are handed over");
    free(text);
    d.busy = 1;
    CHECK(status_composer_dialog_key(&d, 0, '\n') == STATUS_REQUEST_NONE, "a second Enter while posting does nothing");
    d.busy = 0;
    status_composer_dialog_key(&d, 0, '\t');
    CHECK(d.kind == STATUS_KIND_PHOTO, "Tab moves to Photo");
    CHECK(status_composer_dialog_key(&d, 0, 0x0F) == STATUS_REQUEST_CHOOSE_FILE, "Ctrl+O chooses a photo");
    status_composer_dialog_set_file(&d, "/tmp/clip.mp4", STATUS_KIND_VIDEO);
    CHECK(d.kind == STATUS_KIND_VIDEO && strcmp(d.path, "/tmp/clip.mp4") == 0, "a video from the camera switches to Video");
    status_composer_dialog_key(&d, 1, KEY_BTAB);
    CHECK(d.kind == STATUS_KIND_PHOTO, "Shift+Tab goes back");

    /* The camera, on both the Photo and the Video tab. */
    UiRect area = { 1, 0, 28, 100 };
    status_composer_dialog_render(&d, area, "Green", 0xFF25D366, 1);
    CHECK(d.camera_button.w == 14 && status_composer_dialog_click(&d, d.camera_button.y, d.camera_button.x) == STATUS_REQUEST_CAMERA,
          "the Photo tab has Take photo, which opens the camera");
    d.kind = STATUS_KIND_VIDEO;
    status_composer_dialog_render(&d, area, "Green", 0xFF25D366, 1);
    CHECK(d.camera_button.w == 16 && status_composer_dialog_click(&d, d.camera_button.y, d.camera_button.x) == STATUS_REQUEST_CAMERA,
          "the Video tab has Record video, which opens it too");
    status_composer_dialog_render(&d, area, "Green", 0xFF25D366, 0);
    CHECK(d.camera_button.w == 0, "without a camera there is no button");
    d.kind = STATUS_KIND_TEXT;
    status_composer_dialog_render(&d, area, "Green", 0xFF25D366, 1);
    CHECK(d.camera_button.w == 0, "text statuses have none");
    d.kind = STATUS_KIND_PHOTO;
    CHECK(status_composer_dialog_key(&d, 0, 27) == STATUS_REQUEST_CLOSED && !d.open, "Esc closes it");
}

static void test_status_feed(void) {
    StatusAuthor authors[3];
    memset(authors, 0, sizeof(authors));
    strcpy(authors[0].jid, "me@s.whatsapp.net");       authors[0].from_me = 1; authors[0].count = 1; authors[0].latest = 1000;
    strcpy(authors[1].jid, "seen@s.whatsapp.net");     authors[1].count = 2; authors[1].latest = 900;
    strcpy(authors[2].jid, "new@s.whatsapp.net");      authors[2].count = 3; authors[2].unviewed = 2; authors[2].latest = 800;

    StatusFeedDialogs d;
    status_feed_dialogs_open(&d);
    status_list_dialog_render(&d.list, (UiRect){ 1, 0, 28, 100 }, authors, 3, NULL, NULL, 1);
    CHECK(d.list.order[0] == 0 && d.list.order[1] == 2 && d.list.order[2] == 1, "the list shows mine, then unseen, then seen");
    status_feed_dialogs_key(&d, 1, KEY_DOWN);
    CHECK(status_feed_dialogs_key(&d, 0, '\n') == STATUS_FEED_OPEN_AUTHOR && status_feed_dialogs_author(&d) == 2,
          "the second row is the person with unseen statuses");
    CHECK(status_feed_dialogs_key(&d, 0, '+') == STATUS_FEED_POST, "+ in the list starts a new status");

    status_feed_dialogs_view(&d, authors[2].jid, "New", 1, 3);
    CHECK(status_feed_dialogs_viewing(&d) && status_feed_dialogs_index(&d) == 1, "the viewer starts where asked");
    CHECK(status_feed_dialogs_take_moved(&d) && !status_feed_dialogs_take_moved(&d), "the first status counts as moved to, once");
    CHECK(status_feed_dialogs_key(&d, 1, KEY_LEFT) == STATUS_FEED_SHOWING && status_feed_dialogs_index(&d) == 0, "Left steps back");
    CHECK(status_feed_dialogs_key(&d, 1, KEY_LEFT) == STATUS_FEED_NONE, "not before the first");
    CHECK(status_feed_dialogs_key(&d, 0, '\n') == STATUS_FEED_OPEN_MEDIA, "Enter opens the photo or video");
    status_feed_dialogs_key(&d, 1, KEY_RIGHT);
    status_feed_dialogs_key(&d, 1, KEY_RIGHT);
    CHECK(status_feed_dialogs_key(&d, 1, KEY_RIGHT) == STATUS_FEED_REDRAW && !status_feed_dialogs_viewing(&d) &&
          status_feed_dialogs_is_open(&d), "past the last status it goes back to the list");
    CHECK(status_feed_dialogs_key(&d, 0, 27) == STATUS_FEED_CLOSED && !status_feed_dialogs_is_open(&d), "Esc on the list closes it");
}

static void test_new_lines(void) {
    StatusComposerDialog d;
    status_composer_dialog_open(&d, 700);
    status_composer_dialog_key(&d, 0, 'a');
    CHECK(status_composer_dialog_key(&d, 1, TUI_KEY_NEWLINE) == STATUS_REQUEST_REDRAW, "Shift+Enter adds a line in a status");
    status_composer_dialog_key(&d, 0, 'b');
    char *text = status_composer_dialog_text(&d);
    CHECK(text && strcmp(text, "a\nb") == 0, "the status keeps its line break");
    free(text);
    status_composer_dialog_render(&d, (UiRect){ 1, 0, 28, 100 }, "Green", 0xFF25D366, 0);
    CHECK(d.caret.visible && d.caret.y == d.input.scroll_row + (d.last_rect.y + 4) + 1, "the cursor moves to the new line");

    TextField f;
    text_field_init(&f, 100);
    CHECK(!text_field_key(&f, 1, TUI_KEY_NEWLINE), "a one-line box ignores Shift+Enter");
    text_field_paste(&f, "x\ny");
    text = text_field_text(&f);
    CHECK(text && strcmp(text, "x y") == 0, "and pastes line breaks as spaces");
    free(text);
}

static void test_mac_option(void) {
    CHECK(mac_option_letter(0x00AC, 1) == 'l', "Option+L (¬) is Alt+L, even while typing");
    CHECK(mac_option_letter(0x221A, 0) == 'v', "Option+V (√) is Alt+V");
    CHECK(mac_option_letter(0x00F8, 0) == 'o' && mac_option_letter(0x00F8, 1) == 0, "ø is Alt+O only outside text");
    CHECK(mac_option_letter('l', 0) == 0, "plain letters stay letters");
}

static void test_splash(void) {
    SplashView v;
    memset(&v, 0, sizeof(v));
    splash_view_start(&v, 1000);
    CHECK(splash_view_active(&v, 1500), "the splash shows at first");
    splash_view_render(&v, (UiRect){ 0, 0, 30, 100 }, 1500, "v1", "Connecting");
    CHECK(!splash_view_active(&v, 1000 + 60000), "and ends by itself");
    splash_view_start(&v, 0);
    splash_view_skip(&v);
    CHECK(!splash_view_active(&v, 10), "a key skips it");
}

int main(void) {
    setlocale(LC_ALL, "");
    FILE *out = fopen("/dev/null", "w"), *in = fopen("/dev/null", "r");
    SCREEN *screen = out && in ? newterm("xterm-256color", out, in) : NULL;
    if (!screen) { fprintf(stderr, "FAIL: no curses screen\n"); return 1; }
    resizeterm(30, 100);
    tui_palette_init();
    test_header();
    test_profile_dialogs();
    test_status_composer();
    test_status_feed();
    test_splash();
    test_new_lines();
    test_mac_option();
    endwin();
    delscreen(screen);
    if (failures == 0) printf("ok: the header's + and name open the status composer and profile dialogs\n");
    return failures != 0;
}
