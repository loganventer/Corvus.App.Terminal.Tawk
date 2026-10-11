/* What an agent may send as a file: the rules in the policy engine, the
 * look at a real file on disk, and the size as a person reads it. */
#include "core/settings.h"
#include "engines/attachment_policy.h"
#include "engines/byte_size_text.h"
#include "managers/attachment_inspection.h"
#include "utilities/path_util.h"
#include "utilities/str_util.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

static int failures = 0;

#define CHECK(cond, what) do { if (!(cond)) { fprintf(stderr, "FAIL: %s\n", what); failures++; } } while (0)

static FileFacts plain(long long size) {
    FileFacts f = { 1, 1, size };
    return f;
}

static void test_the_rules(void) {
    const char *protected_paths[] = { "/data/tawk/auth", "/data/tawk/tawk.db" };
    AttachmentRules rules = { protected_paths, 2, "/home/logan", 1000 };
    FileFacts ok = plain(500), none = { 0, 0, 0 }, folder = { 1, 0, 4096 }, empty = plain(0), big = plain(5000);

    CHECK(attachment_policy_check("/home/logan/Documents/report.pdf", &ok, &rules) == ATTACHMENT_OK, "an ordinary file may go");
    CHECK(attachment_policy_check("/home/logan/Documents/report.pdf", &none, &rules) == ATTACHMENT_NOT_FOUND, "a file that is not there");
    CHECK(attachment_policy_check("/home/logan/Documents", &folder, &rules) == ATTACHMENT_NOT_A_FILE, "a folder cannot be sent");
    CHECK(attachment_policy_check("/home/logan/Documents/empty.txt", &empty, &rules) == ATTACHMENT_EMPTY, "an empty file is refused");
    CHECK(attachment_policy_check("/home/logan/Documents/big.zip", &big, &rules) == ATTACHMENT_TOO_BIG, "a file over the limit is refused");
    CHECK(attachment_policy_check("/data/tawk/auth/creds.json", &ok, &rules) == ATTACHMENT_PROTECTED, "tawk's login is never sent");
    CHECK(attachment_policy_check("/data/tawk/tawk.db", &ok, &rules) == ATTACHMENT_PROTECTED, "nor its database");
    CHECK(attachment_policy_check("/data/tawk/authentic/notes.txt", &ok, &rules) == ATTACHMENT_OK, "a folder that only starts with the same letters is not covered");
    CHECK(attachment_policy_check("/home/logan/.ssh/config", &ok, &rules) == ATTACHMENT_PROTECTED, "the ssh folder is never sent");
    CHECK(attachment_policy_check("/home/logan/.config/gcloud/credentials.db", &ok, &rules) == ATTACHMENT_PROTECTED, "nor a cloud credentials folder");
    CHECK(attachment_policy_check("/home/logan/.config/other/settings.ini", &ok, &rules) == ATTACHMENT_OK, "another settings folder is fine");
    CHECK(attachment_policy_check("/tmp/work/id_ed25519", &ok, &rules) == ATTACHMENT_PROTECTED, "a key is refused wherever it lies");
    CHECK(attachment_policy_check("/tmp/work/.env", &ok, &rules) == ATTACHMENT_PROTECTED, "and an environment file");
    CHECK(attachment_policy_check("/tmp/work/environment.txt", &ok, &rules) == ATTACHMENT_OK, "a name that only contains .env's letters is fine");
    CHECK(attachment_policy_check("/home/logan/Documents/report.pdf", &ok, NULL) == ATTACHMENT_OK, "with no rules only the file itself is judged");
}

static void test_the_size_text(void) {
    char out[32];
    byte_size_text(0, out, sizeof(out));
    CHECK(!strcmp(out, "0 B"), "zero");
    byte_size_text(812, out, sizeof(out));
    CHECK(!strcmp(out, "812 B"), "bytes");
    byte_size_text(34 * 1024, out, sizeof(out));
    CHECK(!strcmp(out, "34 KB") || !strcmp(out, "34.0 KB"), "kilobytes");
    byte_size_text(1153434, out, sizeof(out));
    CHECK(!strcmp(out, "1.1 MB"), "megabytes with one decimal");
    byte_size_text(3LL * 1024 * 1024 * 1024, out, sizeof(out));
    CHECK(!strcmp(out, "3.0 GB"), "gigabytes");
}

static void write_file(const char *path, const char *body) {
    FILE *f = fopen(path, "wb");
    if (f) { fputs(body, f); fclose(f); }
}

static void test_a_real_file(void) {
    char dir[] = "/tmp/tawk-attach-XXXXXX";
    if (!mkdtemp(dir)) { CHECK(0, "a temporary folder"); return; }
    Settings s;
    settings_set_defaults(&s);
    char data[600], auth[700], file[700], key[700], link[700], inside[700];
    snprintf(data, sizeof(data), "%s/data", dir);
    str_copy(s.data_dir, sizeof(s.data_dir), data);
    settings_auth_dir(&s, auth, sizeof(auth));
    path_mkdir_p(auth, 0700);
    snprintf(inside, sizeof(inside), "%s/creds.json", auth);
    write_file(inside, "{}");
    snprintf(file, sizeof(file), "%s/report.pdf", dir);
    write_file(file, "%PDF-1.4 hello");
    snprintf(key, sizeof(key), "%s/id_rsa", dir);
    write_file(key, "not really a key");
    snprintf(link, sizeof(link), "%s/innocent.txt", dir);
    if (symlink(inside, link) != 0) link[0] = '\0';

    AttachmentInfo info;
    CHECK(attachment_inspection_run(&s, file, &info) == ATTACHMENT_OK, "an ordinary file passes");
    CHECK(!strcmp(info.name, "report.pdf") && !strcmp(info.kind, "document") && info.size_bytes == 14, "and is described: name, kind and size");
    CHECK(attachment_inspection_run(&s, inside, &info) == ATTACHMENT_PROTECTED, "tawk's own login folder is refused");
    CHECK(link[0] && attachment_inspection_run(&s, link, &info) == ATTACHMENT_PROTECTED, "a link that points into it is refused too");
    CHECK(attachment_inspection_run(&s, key, &info) == ATTACHMENT_PROTECTED, "a key by name is refused");
    CHECK(attachment_inspection_run(&s, dir, &info) == ATTACHMENT_NOT_A_FILE, "a folder is refused");
    char missing[700];
    snprintf(missing, sizeof(missing), "%s/nothing.pdf", dir);
    CHECK(attachment_inspection_run(&s, missing, &info) == ATTACHMENT_NOT_FOUND, "a missing file is not found");
    CHECK(attachment_inspection_run(&s, NULL, &info) == ATTACHMENT_NOT_FOUND, "and no path is not found either");

    char command[900];
    snprintf(command, sizeof(command), "rm -rf %s", dir);
    if (system(command) != 0) fprintf(stderr, "note: could not remove %s\n", dir);
}

int main(void) {
    test_the_rules();
    test_the_size_text();
    test_a_real_file();
    if (failures) { fprintf(stderr, "%d check(s) failed\n", failures); return 1; }
    printf("attachment policy: all checks passed\n");
    return 0;
}
