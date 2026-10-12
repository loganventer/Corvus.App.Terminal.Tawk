/* Starting tawk can take the passphrase from a key file the user keeps: it is
 * used once, an unsafe, missing or empty file falls back to the terminal, a
 * new passphrase is always asked for, and a link is never followed. */
#include "infrastructure/stored_passphrase_prompt.h"

#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

static int failures = 0;

#define CHECK(cond, what) do { if (!(cond)) { fprintf(stderr, "FAIL: %s\n", what); failures++; } } while (0)

typedef struct Typed { int asked; } Typed;

static int typed_ask(IPassphrasePrompt *self, const char *prompt, int confirm, Passphrase *out) {
    (void)prompt; (void)confirm;
    ((Typed *)self->ctx)->asked++;
    return passphrase_set(out, "typed-by-hand");
}

static void typed_destroy(IPassphrasePrompt *self) { (void)self; }

static void write_file(const char *path, const char *text, mode_t mode) {
    unlink(path);
    int fd = open(path, O_WRONLY | O_CREAT | O_EXCL, mode);
    if (fd >= 0) {
        if (write(fd, text, strlen(text)) < 0) {}
        close(fd);
    }
    chmod(path, mode);
}

int main(void) {
    char dir[] = "/tmp/tawk-stored-passphrase-XXXXXX";
    if (!mkdtemp(dir)) return 1;
    char key[256], link[256], conf[256];
    snprintf(key, sizeof(key), "%s/db.key", dir);
    snprintf(link, sizeof(link), "%s/link.key", dir);
    snprintf(conf, sizeof(conf), "%s/config.ini", dir);

    Typed typed = { 0 };
    IPassphrasePrompt fallback = { &typed, typed_ask, typed_destroy };
    Passphrase out;

    /* The key file answers, its newline is removed, and the terminal is not asked. */
    write_file(key, "stored-secret\n", 0600);
    IPassphrasePrompt *p = stored_passphrase_prompt_create(&fallback, key);
    CHECK(p != NULL, "prompt is created");
    CHECK(p->ask(p, "Passphrase: ", 0, &out) == 0 && strcmp(out.text, "stored-secret") == 0, "the key file answers, exactly");
    CHECK(typed.asked == 0, "the terminal is not asked when the key file answers");
    passphrase_wipe(&out);

    /* A second question goes to the terminal: a stale file cannot lock anyone out. */
    CHECK(p->ask(p, "Passphrase: ", 0, &out) == 0 && strcmp(out.text, "typed-by-hand") == 0, "second question is typed");
    CHECK(typed.asked == 1, "fallback asked once");
    passphrase_wipe(&out);
    p->destroy(p);

    /* A new passphrase is never taken from the file. */
    typed.asked = 0;
    p = stored_passphrase_prompt_create(&fallback, key);
    CHECK(p->ask(p, "New passphrase: ", 1, &out) == 0 && strcmp(out.text, "typed-by-hand") == 0, "a new passphrase is typed");
    CHECK(typed.asked == 1, "fallback asked for a new passphrase");
    passphrase_wipe(&out);
    p->destroy(p);

    /* The default is db.key beside the settings file. */
    typed.asked = 0;
    p = stored_passphrase_prompt_create_default(&fallback, conf);
    CHECK(p->ask(p, "Passphrase: ", 0, &out) == 0 && strcmp(out.text, "stored-secret") == 0, "default path is db.key beside the settings");
    passphrase_wipe(&out);
    p->destroy(p);

    /* A file others can read is not used. */
    write_file(key, "stored-secret\n", 0644);
    typed.asked = 0;
    p = stored_passphrase_prompt_create(&fallback, key);
    CHECK(p->ask(p, "Passphrase: ", 0, &out) == 0 && strcmp(out.text, "typed-by-hand") == 0, "a group- or world-readable file is ignored");
    CHECK(typed.asked == 1, "fallback asked");
    passphrase_wipe(&out);
    p->destroy(p);

    /* A link is not followed, an empty file and a missing file fall back. */
    write_file(key, "stored-secret\n", 0600);
    unlink(link);
    if (symlink(key, link) != 0) failures++;
    typed.asked = 0;
    p = stored_passphrase_prompt_create(&fallback, link);
    CHECK(p->ask(p, "Passphrase: ", 0, &out) == 0 && strcmp(out.text, "typed-by-hand") == 0, "a link is not followed");
    passphrase_wipe(&out);
    p->destroy(p);

    write_file(key, "\n", 0600);
    p = stored_passphrase_prompt_create(&fallback, key);
    typed.asked = 0;
    CHECK(p->ask(p, "Passphrase: ", 0, &out) == 0 && typed.asked == 1, "an empty file falls back");
    passphrase_wipe(&out);
    p->destroy(p);

    p = stored_passphrase_prompt_create(&fallback, "/no/such/db.key");
    typed.asked = 0;
    CHECK(p->ask(p, "Passphrase: ", 0, &out) == 0 && typed.asked == 1, "a missing file falls back");
    passphrase_wipe(&out);
    p->destroy(p);

    p = stored_passphrase_prompt_create(&fallback, NULL);
    typed.asked = 0;
    CHECK(p->ask(p, "Passphrase: ", 0, &out) == 0 && typed.asked == 1, "no file means typing");
    passphrase_wipe(&out);
    p->destroy(p);

    unlink(key);
    unlink(link);
    rmdir(dir);
    if (failures) { fprintf(stderr, "%d failure(s)\n", failures); return 1; }
    printf("stored_passphrase_test: ok\n");
    return 0;
}
