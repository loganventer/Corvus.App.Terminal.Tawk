/* The notification sounds to choose from: which files count, how the folders
 * combine, and how the picker moves through them. */
#include "clients/tui/sound_picker.h"
#include "engines/sound_name.h"
#include "resource_access/folder_sound_catalog.h"
#include "utilities/path_util.h"

#include <ncurses.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

static int failures = 0;

#define CHECK(cond, what) do { if (!(cond)) { fprintf(stderr, "FAIL: %s\n", what); failures++; } } while (0)

static void touch(const char *dir, const char *name) {
    char path[600];
    path_join(path, sizeof(path), dir, name);
    FILE *f = fopen(path, "wb");
    if (f) { fputs("x", f); fclose(f); }
}

static void test_which_files_are_sounds(void) {
    CHECK(sound_name_is_playable("ping.wav"), "a wav file");
    CHECK(sound_name_is_playable("PING.OGG"), "an ogg file in capitals");
    CHECK(!sound_name_is_playable("notes.txt"), "a text file is not");
    CHECK(!sound_name_is_playable("wav"), "a name without an extension is not");

    char label[64];
    sound_name_label("/usr/share/corvus-tawk/sounds/bong.wav", label, sizeof(label));
    CHECK(strcmp(label, "bong") == 0, "the label drops folder and extension");
    sound_name_label("plain", label, sizeof(label));
    CHECK(strcmp(label, "plain") == 0, "a bare name stays as it is");
}

static void test_the_folders_combine(char *bundled, char *user) {
    touch(bundled, "pluck.wav");
    touch(bundled, "bong.wav");
    touch(bundled, "readme.md");
    touch(user, "bong.ogg");
    touch(user, "ding.wav");

    ISoundCatalog *c = folder_sound_catalog_create(bundled, user);
    CHECK(c != NULL, "the catalog is created");
    CHECK(c->count(c) == 3, "only sound files count, a name once");
    CHECK(strcmp(c->at(c, 0)->id, "bong") == 0, "sorted by name");
    CHECK(strstr(c->at(c, 0)->path, "bong.ogg") != NULL, "your file replaces the bundled one of the same name");
    CHECK(c->index_of_id(c, "pluck") == 2, "found by id");
    CHECK(c->index_of_id(c, "nope") == -1, "an unknown id");

    char path[600];
    path_join(path, sizeof(path), bundled, "pluck.wav");
    CHECK(c->index_of_path(c, path) == 2, "found by path");
    CHECK(c->index_of_path(c, "/somewhere/else.wav") == -1, "a file of your own is not in the catalog");

    touch(user, "alpha.wav");
    CHECK(c->count(c) == 3, "a new file is not seen before a reload");
    c->reload(c);
    CHECK(c->count(c) == 4 && strcmp(c->at(c, 0)->id, "alpha") == 0, "a reload picks it up");
    c->destroy(c);

    ISoundCatalog *none = folder_sound_catalog_create("/no/such/folder", NULL);
    CHECK(none && none->count(none) == 0, "missing folders give an empty catalog");
    none->destroy(none);
}

static void test_the_picker_moves(char *bundled, char *user) {
    ISoundCatalog *c = folder_sound_catalog_create(bundled, user);
    char path[600];
    path_join(path, sizeof(path), bundled, "pluck.wav");

    SoundPicker p;
    sound_picker_begin(&p, c, path);
    CHECK(p.active && p.index == c->index_of_path(c, path), "opens on the sound in use");
    CHECK(sound_picker_key(&p, c, 1, KEY_DOWN) == SOUND_PICKER_OPEN, "the last row cannot go further");
    CHECK(sound_picker_key(&p, c, 1, KEY_UP) == SOUND_PICKER_MOVED, "up moves");
    CHECK(sound_picker_key(&p, c, 1, KEY_HOME) == SOUND_PICKER_MOVED && p.index == 0, "home goes to the first");
    CHECK(sound_picker_current(&p, c) == c->at(c, 0), "the current sound is the highlighted one");
    CHECK(sound_picker_wheel(&p, c, 2) == SOUND_PICKER_MOVED && p.index == 2, "the wheel moves");

    CHECK(sound_picker_key(&p, c, 0, '\n') == SOUND_PICKER_CHOSEN && !p.active, "enter chooses and closes");
    CHECK(sound_picker_current(&p, c) == c->at(c, 2), "the choice stays readable after closing");

    sound_picker_begin(&p, c, "/own/file.wav");
    CHECK(p.index == 0, "a file of your own starts at the first sound");
    CHECK(sound_picker_key(&p, c, 0, 27) == SOUND_PICKER_CLOSED && !p.active, "escape closes without choosing");

    sound_picker_begin(&p, c, path);
    p.list_y = 5;
    p.list_h = 10;
    p.scroll = 0;
    CHECK(sound_picker_click(&p, c, 4) == SOUND_PICKER_OPEN, "a click above the list does nothing");
    CHECK(sound_picker_click(&p, c, 5) == SOUND_PICKER_MOVED && p.index == 0, "a click highlights a row");
    CHECK(sound_picker_click(&p, c, 5) == SOUND_PICKER_CHOSEN, "a second click on it chooses");
    CHECK(sound_picker_click(&p, c, 5) == SOUND_PICKER_CLOSED, "a closed picker ignores clicks");
    c->destroy(c);

    ISoundCatalog *none = folder_sound_catalog_create("/no/such/folder", NULL);
    sound_picker_begin(&p, none, "");
    CHECK(sound_picker_current(&p, none) == NULL, "nothing to highlight in an empty catalog");
    CHECK(sound_picker_key(&p, none, 0, '\n') == SOUND_PICKER_CLOSED, "choosing from nothing just closes");
    none->destroy(none);
}

int main(void) {
    char root[] = "/tmp/tawk-sound-picker-XXXXXX";
    if (!mkdtemp(root)) { perror("mkdtemp"); return 1; }
    char bundled[600], user[600];
    path_join(bundled, sizeof(bundled), root, "bundled");
    path_join(user, sizeof(user), root, "user");
    path_mkdir_p(bundled, 0700);
    path_mkdir_p(user, 0700);

    test_which_files_are_sounds();
    test_the_folders_combine(bundled, user);
    test_the_picker_moves(bundled, user);

    char cmd[700];
    snprintf(cmd, sizeof(cmd), "rm -rf '%s'", root);
    if (system(cmd) != 0) fprintf(stderr, "could not remove %s\n", root);

    if (failures) { fprintf(stderr, "%d check(s) failed\n", failures); return 1; }
    printf("sound picker: all checks passed\n");
    return 0;
}
