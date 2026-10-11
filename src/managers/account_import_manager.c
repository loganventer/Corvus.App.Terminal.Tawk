#include "managers/account_import_manager.h"
#include "engines/account_import_policy.h"
#include "engines/account_package_manifest_codec.h"
#include "engines/account_package_path_policy.h"
#include "engines/media_path_rewriter.h"
#include "utilities/file_copy.h"
#include "utilities/log.h"
#include "utilities/path_util.h"
#include "utilities/staging_dir.h"
#include "utilities/str_util.h"
#include "utilities/tree_copy.h"

#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>

#define PATH_SIZE   1200
#define MAX_MEMBERS 30000

struct AccountImportManager {
    AccountImportManagerDeps deps;
};

AccountImportManager *account_import_manager_create(const AccountImportManagerDeps *deps) {
    AccountImportManager *m = calloc(1, sizeof(*m));
    if (m) m->deps = *deps;
    return m;
}

void account_import_manager_destroy(AccountImportManager *m) { free(m); }

int account_import_manager_available(AccountImportManager *m) { return m->deps.cipher->available(m->deps.cipher); }

static int refuse(char *why, unsigned long size, const char *text) {
    str_copy(why, size, text);
    return -1;
}

static int read_small_file(const char *path, char *out, size_t size) {
    int fd = open(path, O_RDONLY | O_CLOEXEC | O_NOFOLLOW);
    if (fd < 0) return -1;
    ssize_t n = read(fd, out, size - 1);
    close(fd);
    if (n < 0) return -1;
    out[n] = '\0';
    return 0;
}

/* Every member must be one a package holds; nothing is unpacked otherwise. */
static int check_members(AccountImportManager *m, const char *archive, char *why, unsigned long size) {
    ArchiveEntry *entries = calloc(MAX_MEMBERS, sizeof(*entries));
    if (!entries) return refuse(why, size, "out of memory");
    int n = m->deps.archive->list(m->deps.archive, archive, entries, MAX_MEMBERS);
    int rc = n > 0 ? 0 : refuse(why, size, "the package cannot be read (damaged, or not made by tawk)");
    for (int i = 0; rc == 0 && i < n; i++) {
        if (!account_package_path_allowed(&entries[i])) {
            char text[1300];
            snprintf(text, sizeof(text), "the package holds something tawk never writes (%s); nothing was imported", entries[i].name);
            rc = refuse(why, size, text);
        }
    }
    free(entries);
    return rc;
}

typedef struct MediaPlace {
    const char *source_media;     /* the old media folder, as the package names it */
    const char *content_media;    /* media/ inside the unpacked package */
    const char *new_media;        /* this computer's media folder */
} MediaPlace;

static void place_one_media(void *ctx, const char *path) {
    MediaPlace *p = ctx;
    char rel[PATH_SIZE], from[PATH_SIZE], to[PATH_SIZE];
    if (media_path_relative(p->source_media, path, rel, sizeof(rel)) != 0) return;
    snprintf(from, sizeof(from), "%s/%s", p->content_media, rel);
    snprintf(to, sizeof(to), "%s/%s", p->new_media, rel);
    if (!path_is_regular_file(from) || path_is_regular_file(to)) return;     /* not carried, or already here */
    char dir[PATH_SIZE];
    str_copy(dir, sizeof(dir), to);
    char *slash = strrchr(dir, '/');
    if (slash) { *slash = '\0'; path_mkdir_p(dir, 0700); }
    file_copy(from, to, 0600);
}

int account_import_manager_import(AccountImportManager *m, const AccountImportRequest *r, const Passphrase *passphrase,
                                  MigrationReport *report, Account *created, char *why, unsigned long size) {
    memset(report, 0, sizeof(*report));
    if (!account_import_manager_available(m)) return refuse(why, size, "openssl is not installed, so the package cannot be decrypted");
    struct stat st;
    if (lstat(r->input, &st) != 0 || !S_ISREG(st.st_mode)) return refuse(why, size, "there is no such package file");

    char staging[PATH_SIZE], content[PATH_SIZE], archive[PATH_SIZE], manifest_path[PATH_SIZE], package[PATH_SIZE], media[PATH_SIZE];
    path_mkdir_p(r->paths.data_dir, 0700);
    if (staging_dir_make(r->paths.data_dir, "import", staging, sizeof(staging)) != 0) {
        return refuse(why, size, "cannot make a working folder in the data folder");
    }
    snprintf(content, sizeof(content), "%s/content", staging);
    snprintf(archive, sizeof(archive), "%s/package.tar.gz", staging);
    snprintf(manifest_path, sizeof(manifest_path), "%s/account-manifest.txt", content);
    snprintf(package, sizeof(package), "%s/package.db", content);
    snprintf(media, sizeof(media), "%s/media", content);

    int rc = -1;
    char text[2048];
    AccountPackageManifest manifest;
    int checked = m->deps.cipher->checked && m->deps.cipher->checked(m->deps.cipher, r->input);
    if (m->deps.cipher->checked && !checked) {
        refuse(why, size, "the check that guards this package against changes is missing from it; nothing was imported");
    } else if (m->deps.cipher->decrypt(m->deps.cipher, r->input, archive, passphrase) != 0) {
        refuse(why, size, "the passphrase is wrong, or the package was changed or damaged, or this is not a tawk package");
    } else if (check_members(m, archive, why, size) != 0) {
        /* why is set */
    } else if (mkdir(content, 0700) != 0 || m->deps.archive->extract(m->deps.archive, archive, content) != 0) {
        refuse(why, size, "tar could not unpack the package");
    } else if (read_small_file(manifest_path, text, sizeof(text)) != 0 || account_package_manifest_parse(text, &manifest) != 0) {
        refuse(why, size, "the package was made by a version of tawk this one cannot read");
    } else if (!path_is_regular_file(package)) {
        refuse(why, size, "the package holds no chats");
    } else {
        Account existing[ACCOUNT_MAX];
        int have = m->deps.accounts->list(m->deps.accounts, existing, ACCOUNT_MAX);
        ImportVerdict verdict = account_import_policy_check(&manifest, m->deps.schema->version(m->deps.schema, NULL), have);
        char label[ACCOUNT_LABEL_SIZE];
        if (verdict != IMPORT_OK) {
            refuse(why, size, import_verdict_reason(verdict));
        } else if (account_import_policy_label(r->label && *r->label ? r->label : manifest.label, existing, have, label, sizeof(label)) != 0) {
            refuse(why, size, "could not find a free label for the account; give one with --label");
        } else {
            AccountId id = ACCOUNT_ID_NONE;
            if (m->deps.accounts->add(m->deps.accounts, label, manifest.colour, (int64_t)time(NULL), &id) != 0) {
                refuse(why, size, "could not add the account");
            } else {
                MergePlan plan = { manifest.account_id, id, { manifest.media_dir, r->paths.media_dir, manifest.has_media } };
                if (m->deps.merger->merge(m->deps.merger, package, &plan, report) != 0) {
                    m->deps.accounts->remove(m->deps.accounts, id);
                    snprintf(text, sizeof(text), "the chats could not be put in the database (%s); nothing was changed", m->deps.merger->error(m->deps.merger));
                    refuse(why, size, text);
                } else {
                    if (manifest.has_media && r->paths.media_dir) {
                        MediaPlace place = { manifest.media_dir, media, r->paths.media_dir };
                        m->deps.extractor->media_paths(m->deps.extractor, package, place_one_media, &place);
                    }
                    if (manifest.jid[0]) m->deps.expectation->expect(m->deps.expectation, id, manifest.jid);
                    if (created) m->deps.accounts->get(m->deps.accounts, id, created);
                    rc = 0;
                    LOG_INFO("account package imported (%d messages)", report->messages);
                }
            }
        }
    }
    tree_remove(staging);
    return rc;
}
