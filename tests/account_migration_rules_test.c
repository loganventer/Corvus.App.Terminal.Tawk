/* The rules behind moving an account: which tables travel, which package
 * may be brought in and under what label, which number may link a moved
 * account, how media paths change, and which members of a package may be
 * unpacked. All pure, so none of it needs a database. */
#include "core/account.h"
#include "core/account_package_manifest.h"
#include "engines/account_identity_rule.h"
#include "engines/account_import_policy.h"
#include "engines/account_package_manifest_codec.h"
#include "engines/account_package_path_policy.h"
#include "engines/media_path_rewriter.h"
#include "engines/table_catalog.h"
#include "utilities/str_util.h"

#include <stdio.h>
#include <string.h>

static int failures = 0;

#define CHECK(cond, what) do { if (!(cond)) { fprintf(stderr, "FAIL: %s\n", what); failures++; } } while (0)

static void test_catalog(void) {
    CHECK(table_catalog_scope("messages") == TABLE_SCOPE_ACCOUNT, "messages travel with their account");
    CHECK(table_catalog_scope("chat_labels") == TABLE_SCOPE_BY_JID, "labels follow the account's chats");
    CHECK(table_catalog_scope("scheduled_messages") == TABLE_SCOPE_LEFT_BEHIND, "messages waiting for a time stay behind");
    CHECK(table_catalog_scope("messages_fts_data") == TABLE_SCOPE_GLOBAL && table_catalog_scope("sqlite_sequence") == TABLE_SCOPE_GLOBAL,
          "the search index and SQLite's own tables are rebuilt, not carried");
    CHECK(table_catalog_scope("a_new_table") == TABLE_SCOPE_UNKNOWN, "a table nobody decided about is unknown");
    char names[3][64] = { "messages", "a_new_table", "another_one" };
    const char *first = NULL;
    CHECK(table_catalog_unclassified(names, 3, &first) == 2 && first && !strcmp(first, "a_new_table"), "unclassified tables are counted and named");
    int n = 0;
    const TableRule *rules = table_catalog_rules(&n);
    int by_jid_has_column = 1;
    for (int i = 0; i < n; i++) if (rules[i].scope == TABLE_SCOPE_BY_JID && !rules[i].key_column) by_jid_has_column = 0;
    CHECK(by_jid_has_column, "every table taken by chat names the column that holds the chat");
}

static void test_import_policy(void) {
    AccountPackageManifest m;
    memset(&m, 0, sizeof(m));
    m.format = ACCOUNT_PACKAGE_FORMAT;
    m.schema_version = 26;
    CHECK(account_import_policy_check(&m, 26, 2) == IMPORT_OK, "a package of the same schema is welcome");
    CHECK(account_import_policy_check(&m, 30, 2) == IMPORT_OK, "and one from an older tawk");
    CHECK(account_import_policy_check(&m, 25, 2) == IMPORT_NEWER_SCHEMA, "one from a newer tawk is refused");
    m.format = 99;
    CHECK(account_import_policy_check(&m, 26, 2) == IMPORT_BAD_MANIFEST, "a format it does not know is refused");
    m.format = ACCOUNT_PACKAGE_FORMAT;
    CHECK(account_import_policy_check(&m, 26, ACCOUNT_MAX) == IMPORT_NO_ROOM, "a full tawk has no room");

    Account have[2];
    memset(have, 0, sizeof(have));
    str_copy(have[0].label, sizeof(have[0].label), "Personal");
    str_copy(have[1].label, sizeof(have[1].label), "Personal 2");
    char out[ACCOUNT_LABEL_SIZE];
    CHECK(account_import_policy_label("Work", have, 2, out, sizeof(out)) == 0 && !strcmp(out, "Work"), "a free label is kept");
    CHECK(account_import_policy_label("personal", have, 2, out, sizeof(out)) == 0 && !strcmp(out, "personal 3"), "a taken label gets the next free number");
    CHECK(account_import_policy_label("", have, 2, out, sizeof(out)) == 0 && !strcmp(out, "Moved"), "no label starts from 'Moved'");
    CHECK(account_import_policy_label("a,b", have, 2, out, sizeof(out)) == 0 && !strcmp(out, "Moved"), "a label that is not allowed starts from 'Moved'");
    char long_name[] = "abcdefghijklmnopqrstuvwx";   /* 24 characters */
    Account one;
    memset(&one, 0, sizeof(one));
    str_copy(one.label, sizeof(one.label), long_name);
    CHECK(account_import_policy_label(long_name, &one, 1, out, sizeof(out)) == 0 && !strcmp(out, "abcdefghijklmnopqrstuv 2"),
          "a taken label of full length is cut to make room for the number");
}

static void test_identity(void) {
    CHECK(account_identity_rule("", "27821234567@s.whatsapp.net") == IDENTITY_NOT_EXPECTED, "an account that was not moved is not checked");
    CHECK(account_identity_rule("27821234567@s.whatsapp.net", "27821234567:12@s.whatsapp.net") == IDENTITY_MATCH, "the same number on another device matches");
    CHECK(account_identity_rule("27821234567@s.whatsapp.net", "27829999999@s.whatsapp.net") == IDENTITY_MISMATCH, "another number does not");
    CHECK(account_identity_rule("27821234567@s.whatsapp.net", "") == IDENTITY_MISMATCH, "nobody at all does not");
    CHECK(account_identity_rule("2782123456@s.whatsapp.net", "27821234567@s.whatsapp.net") == IDENTITY_MISMATCH, "a number that is a prefix of another is not it");
}

static void test_media_paths(void) {
    MediaPathMap map = { "/old/media", "/new/cache/media", 1 };
    char out[256];
    CHECK(media_path_rewrite(&map, "/old/media/outgoing/AB12.pdf", out, sizeof(out)) == 0 && !strcmp(out, "/new/cache/media/outgoing/AB12.pdf"),
          "a file in the old media folder moves to the new one");
    CHECK(media_path_rewrite(&map, "/elsewhere/AB12.pdf", out, sizeof(out)) == -1, "a path outside it is cleared");
    CHECK(media_path_rewrite(&map, "/old/media/../secrets", out, sizeof(out)) == -1, "a path that climbs out is cleared");
    CHECK(media_path_rewrite(&map, "/old/mediaX/a.jpg", out, sizeof(out)) == -1, "a folder that only starts with the same letters is not inside it");
    MediaPathMap left = { "/old/media", "/new/media", 0 };
    CHECK(media_path_rewrite(&left, "/old/media/a.jpg", out, sizeof(out)) == -1, "paths are cleared when the files did not come");
    CHECK(media_path_relative("/old/media/", "/old/media/a/b.jpg", out, sizeof(out)) == 0 && !strcmp(out, "a/b.jpg"), "the relative part is what is below the folder");
}

static void test_package_paths(void) {
    ArchiveEntry e;
    memset(&e, 0, sizeof(e));
    str_copy(e.name, sizeof(e.name), "package.db");  e.type = '-';
    CHECK(account_package_path_allowed(&e), "the database file may be unpacked");
    str_copy(e.name, sizeof(e.name), "account-manifest.txt");
    CHECK(account_package_path_allowed(&e), "so may the description");
    str_copy(e.name, sizeof(e.name), "media/outgoing/AB12.pdf");
    CHECK(account_package_path_allowed(&e), "and files under media");
    str_copy(e.name, sizeof(e.name), "media/../../etc/passwd");
    CHECK(!account_package_path_allowed(&e), "but not a path that climbs out");
    str_copy(e.name, sizeof(e.name), "/etc/passwd");
    CHECK(!account_package_path_allowed(&e), "nor an absolute one");
    str_copy(e.name, sizeof(e.name), "tawk.db");
    CHECK(!account_package_path_allowed(&e), "nor a backup's database");
    str_copy(e.name, sizeof(e.name), "auth");  e.type = 'd';
    CHECK(!account_package_path_allowed(&e), "nor a login folder");
    str_copy(e.name, sizeof(e.name), "package.db");  e.type = 'l';
    CHECK(!account_package_path_allowed(&e), "nor a link");
}

static void test_manifest(void) {
    AccountPackageManifest a, b;
    memset(&a, 0, sizeof(a));
    a.format = ACCOUNT_PACKAGE_FORMAT;
    str_copy(a.app_version, sizeof(a.app_version), "0.23.0");
    a.schema_version = 27;
    a.created = 1790000000;
    a.account_id = 2;
    str_copy(a.label, sizeof(a.label), "Personal");
    str_copy(a.jid, sizeof(a.jid), "27609742113@s.whatsapp.net");
    str_copy(a.name, sizeof(a.name), "Logan");
    a.colour = 3;
    str_copy(a.media_dir, sizeof(a.media_dir), "/home/logan/.cache/tawk/media");
    a.has_media = 1;
    char text[1500];
    CHECK(account_package_manifest_format(&a, text, sizeof(text)) == 0, "a manifest is written");
    CHECK(account_package_manifest_parse(text, &b) == 0, "and read back");
    CHECK(b.schema_version == 27 && b.account_id == 2 && b.colour == 3 && b.has_media && !strcmp(b.label, "Personal") &&
              !strcmp(b.jid, "27609742113@s.whatsapp.net") && !strcmp(b.media_dir, "/home/logan/.cache/tawk/media"),
          "with everything in it");
    CHECK(account_package_manifest_parse("format=7\n", &b) != 0, "a format from the future is refused");
    CHECK(account_package_manifest_parse("nothing here", &b) != 0, "text that is no manifest is refused");
}

int main(void) {
    test_catalog();
    test_import_policy();
    test_identity();
    test_media_paths();
    test_package_paths();
    test_manifest();
    if (failures) { fprintf(stderr, "%d check(s) failed\n", failures); return 1; }
    printf("account migration rules: all checks passed\n");
    return 0;
}
