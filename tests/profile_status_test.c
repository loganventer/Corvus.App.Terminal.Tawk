/* Your own profile and statuses: the checks, the protocol, and the account
 * and status managers against fake backends. */
#include "core/event.h"
#include "engines/profile_field_validator.h"
#include "engines/status_background_palette.h"
#include "engines/status_post_validator.h"
#include "engines/url_finder.h"
#include "managers/account_manager.h"
#include "managers/status_manager.h"
#include "resource_access/json_protocol.h"
#include "utilities/str_util.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

static int failures = 0;

#define CHECK(cond, what) do { if (!(cond)) { fprintf(stderr, "FAIL: %s\n", what); failures++; } } while (0)

static char media_dir[] = "/tmp/tawk-profile-status-test-XXXXXX";

static void write_file(const char *path, const char *content) {
    FILE *f = fopen(path, "w");
    if (!f) return;
    fputs(content, f);
    fclose(f);
}

static void test_validators(void) {
    char why[256];
    CHECK(profile_field_validate(PROFILE_FIELD_NAME, "Logan", why, sizeof(why)) == 0, "a short name is fine");
    CHECK(profile_field_validate(PROFILE_FIELD_NAME, "   ", why, sizeof(why)) != 0 && why[0], "a blank name is refused");
    CHECK(profile_field_validate(PROFILE_FIELD_NAME, "abcdefghijklmnopqrstuvwxyz", why, sizeof(why)) != 0,
          "a 26 character name is refused");
    /* 25 two-byte characters: counted as characters, not bytes */
    char accents[128] = "";
    for (int i = 0; i < 25; i++) strcat(accents, "\xC3\xA9");
    CHECK(profile_field_validate(PROFILE_FIELD_NAME, accents, why, sizeof(why)) == 0, "names are counted in characters");
    CHECK(profile_field_validate(PROFILE_FIELD_ABOUT, "", why, sizeof(why)) == 0, "an empty about is allowed");
    char about[200];
    memset(about, 'a', 140);
    about[140] = '\0';
    CHECK(profile_field_validate(PROFILE_FIELD_ABOUT, about, why, sizeof(why)) != 0, "a 140 character about is refused");
    CHECK(profile_field_max_chars(PROFILE_FIELD_ABOUT) == 139, "about holds 139 characters");

    StatusPost post;
    memset(&post, 0, sizeof(post));
    post.kind = STATUS_KIND_TEXT;
    CHECK(status_post_validate(&post, why, sizeof(why)) != 0, "an empty text status is refused");
    str_copy(post.text, sizeof(post.text), "Hello");
    CHECK(status_post_validate(&post, why, sizeof(why)) == 0, "a text status is fine");
    post.kind = STATUS_KIND_LINK;
    CHECK(status_post_validate(&post, why, sizeof(why)) != 0, "a link status needs an address");
    str_copy(post.text, sizeof(post.text), "Look at https://example.com/page.");
    CHECK(status_post_validate(&post, why, sizeof(why)) == 0, "a link status with an address is fine");
    post.kind = STATUS_KIND_PHOTO;
    CHECK(status_post_validate(&post, why, sizeof(why)) != 0, "a photo status needs a file");
    char photo[512], notes[512];
    snprintf(photo, sizeof(photo), "%s/photo.jpg", media_dir);
    snprintf(notes, sizeof(notes), "%s/notes.txt", media_dir);
    write_file(photo, "not really a jpeg");
    write_file(notes, "text");
    str_copy(post.path, sizeof(post.path), photo);
    CHECK(status_post_validate(&post, why, sizeof(why)) == 0, "a photo status with a photo is fine");
    post.kind = STATUS_KIND_VIDEO;
    CHECK(status_post_validate(&post, why, sizeof(why)) != 0, "a photo is not a video");
    post.kind = STATUS_KIND_PHOTO;
    str_copy(post.path, sizeof(post.path), notes);
    CHECK(status_post_validate(&post, why, sizeof(why)) != 0, "a text file is not a photo");
}

static void test_url_finder(void) {
    char url[256];
    CHECK(url_find_first("see https://example.com/a?b=1, thanks", url, sizeof(url)) == 0 &&
          strcmp(url, "https://example.com/a?b=1") == 0, "the address stops before trailing punctuation");
    CHECK(url_find_first("(http://x.org)", url, sizeof(url)) == 0 && strcmp(url, "http://x.org") == 0, "brackets are left out");
    CHECK(url_find_first("no address here", url, sizeof(url)) != 0, "text without an address has none");
    CHECK(url_find_first("xhttps://a.b", url, sizeof(url)) != 0, "an address must start a word");
    CHECK(status_background_count() > 1 && status_background_at(status_background_count()) == status_background_at(0) &&
          status_background_name(-1)[0], "the palette wraps around");
}

static void test_protocol(void) {
    char *json = json_protocol_encode_set_name("Logan");
    CHECK(json && strstr(json, "\"cmd\":\"set_name\"") && strstr(json, "\"name\":\"Logan\""), "set_name is encoded");
    free(json);
    json = json_protocol_encode_remove_picture();
    CHECK(json && strstr(json, "\"cmd\":\"remove_picture\""), "remove_picture is encoded");
    free(json);
    StatusPost post;
    memset(&post, 0, sizeof(post));
    post.kind = STATUS_KIND_TEXT;
    post.background_argb = 0xFF128C7E;
    str_copy(post.text, sizeof(post.text), "Hi");
    str_copy(post.id, sizeof(post.id), "3EB0AA");
    json = json_protocol_encode_post_status(&post);
    CHECK(json && strstr(json, "\"cmd\":\"post_status\"") && strstr(json, "\"kind\":\"text\"") &&
          strstr(json, "\"bg\":4279405694") && strstr(json, "\"id\":\"3EB0AA\""), "post_status is encoded");
    free(json);

    Event e;
    event_init(&e, EVENT_NONE);
    CHECK(json_protocol_decode("{\"evt\":\"profile_updated\",\"field\":\"name\",\"ok\":true,\"name\":\"Lo\"}", &e) == 0 &&
          e.type == EVENT_PROFILE_UPDATED && !strcmp(e.reason, "name") && e.ok && !strcmp(e.name, "Lo"),
          "profile_updated is decoded");
    event_dispose(&e);
    event_init(&e, EVENT_NONE);
    CHECK(json_protocol_decode("{\"evt\":\"status_posted\",\"id\":\"3EB0AA\",\"ok\":false,\"detail\":\"no\"}", &e) == 0 &&
          e.type == EVENT_STATUS_POSTED && !strcmp(e.id, "3EB0AA") && !e.ok && !strcmp(e.detail, "no"),
          "status_posted is decoded");
    event_dispose(&e);
}

/* ---- managers with fake backends ---------------------------------------- */

static char last_name[128], last_picture[600];
static int removes;
static StatusPost last_post;

static int fake_set_name(IProfileEditor *self, const char *name) { (void)self; str_copy(last_name, sizeof(last_name), name); return 0; }
static int fake_set_about(IProfileEditor *self, const char *text) { (void)self; (void)text; return 0; }
static int fake_set_picture(IProfileEditor *self, const char *path) { (void)self; str_copy(last_picture, sizeof(last_picture), path); return 0; }
static int fake_remove_picture(IProfileEditor *self) { (void)self; removes++; return 0; }
static int fake_post(IStatusPublisher *self, const StatusPost *post) { (void)self; last_post = *post; return 0; }

static void deliver(IEventObserver *o, Event *e) {
    o->on_event(o, e);
    event_dispose(e);
}

static void test_account_manager(void) {
    IProfileEditor editor = { NULL, fake_set_name, fake_set_about, fake_set_picture, fake_remove_picture };
    AccountManagerDeps deps = { &editor, media_dir };
    AccountManager *m = account_manager_create(&deps);
    IEventObserver *o = account_manager_observer(m);
    Event e;
    event_init(&e, EVENT_AUTH_CONNECTED);
    str_copy(e.jid, sizeof(e.jid), "27821234567@s.whatsapp.net");
    str_copy(e.name, sizeof(e.name), "Old");
    deliver(o, &e);
    CHECK(!strcmp(account_manager_user_name(m), "Old") && account_manager_user_jid(m)[0], "the linked account is known");

    CHECK(account_manager_set_name(m, "") != 0 && account_manager_error(m)[0], "an empty name is not sent");
    CHECK(account_manager_set_name(m, "New") == 0 && !strcmp(last_name, "New"), "a name is sent");
    CHECK(account_manager_busy(m, PROFILE_FIELD_NAME), "the name is busy until WhatsApp answers");
    CHECK(account_manager_set_name(m, "Other") != 0, "one name change at a time");
    event_init(&e, EVENT_PROFILE_UPDATED);
    str_copy(e.reason, sizeof(e.reason), "name");
    e.ok = 1;
    deliver(o, &e);
    ProfileEditResult r;
    CHECK(!account_manager_busy(m, PROFILE_FIELD_NAME) && !strcmp(account_manager_user_name(m), "New"), "a saved name is shown");
    CHECK(account_manager_take_result(m, &r) == 0 && r.field == PROFILE_FIELD_NAME && r.ok, "the result is handed over");
    CHECK(account_manager_take_result(m, &r) != 0, "once");

    char photo[512];
    snprintf(photo, sizeof(photo), "%s/photo.jpg", media_dir);
    CHECK(account_manager_set_picture(m, photo) == 0 && strstr(last_picture, "/outgoing/") != NULL,
          "a picture is copied into the media folder before it is sent");
    event_init(&e, EVENT_PROFILE_UPDATED);
    str_copy(e.reason, sizeof(e.reason), "picture");
    str_copy(e.detail, sizeof(e.detail), "too small");
    deliver(o, &e);
    CHECK(account_manager_take_result(m, &r) == 0 && !r.ok && !strcmp(r.detail, "too small"), "a refusal is reported");
    CHECK(account_manager_remove_picture(m) == 0 && removes == 1, "the picture can be removed");
    account_manager_destroy(m);
}

static void test_status_manager(void) {
    StatusManagerDeps none = { NULL, media_dir };
    StatusManager *m = status_manager_create(&none);
    StatusPost post;
    memset(&post, 0, sizeof(post));
    post.kind = STATUS_KIND_TEXT;
    str_copy(post.text, sizeof(post.text), "Hello");
    CHECK(!status_manager_supported(m) && status_manager_post(m, &post) != 0 &&
          strstr(status_manager_error(m), "whatsmeow") != NULL, "without a publisher statuses are not supported");
    status_manager_destroy(m);

    IStatusPublisher publisher = { NULL, fake_post };
    StatusManagerDeps deps = { &publisher, media_dir };
    m = status_manager_create(&deps);
    CHECK(status_manager_supported(m), "with a publisher statuses are supported");
    CHECK(status_manager_post(m, &post) == 0 && last_post.id[0] && !strcmp(last_post.text, "Hello"), "a text status is sent");
    CHECK(status_manager_busy(m) && status_manager_post(m, &post) != 0, "one post at a time");
    Event e;
    event_init(&e, EVENT_STATUS_POSTED);
    str_copy(e.id, sizeof(e.id), "someone-else");
    e.ok = 1;
    deliver(status_manager_observer(m), &e);
    CHECK(status_manager_busy(m), "an answer for another post is ignored");
    event_init(&e, EVENT_STATUS_POSTED);
    str_copy(e.id, sizeof(e.id), last_post.id);
    e.ok = 1;
    deliver(status_manager_observer(m), &e);
    StatusPostResult r;
    CHECK(!status_manager_busy(m) && status_manager_take_result(m, &r) == 0 && r.ok, "a posted status is reported");

    post.kind = STATUS_KIND_PHOTO;
    snprintf(post.path, sizeof(post.path), "%s/photo.jpg", media_dir);
    CHECK(status_manager_post(m, &post) == 0 && strstr(last_post.path, "/outgoing/") != NULL &&
          !strcmp(last_post.mime, "image/jpeg"), "a photo is copied into the media folder with its type");
    status_manager_destroy(m);
}

int main(void) {
    if (!mkdtemp(media_dir)) { perror("mkdtemp"); return 1; }
    test_validators();
    test_url_finder();
    test_protocol();
    test_account_manager();
    test_status_manager();
    char cmd[128];
    snprintf(cmd, sizeof(cmd), "rm -rf '%s'", media_dir);
    if (system(cmd) != 0) fprintf(stderr, "could not remove %s\n", media_dir);
    if (failures == 0) printf("ok: profile edits and statuses are checked, sent and reported\n");
    return failures != 0;
}
