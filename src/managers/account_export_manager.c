#include "managers/account_export_manager.h"
#include "engines/account_label_validator.h"
#include "engines/account_package_manifest_codec.h"
#include "engines/media_path_rewriter.h"
#include "utilities/app_info.h"
#include "utilities/file_copy.h"
#include "utilities/log.h"
#include "utilities/path_util.h"
#include "utilities/staging_dir.h"
#include "utilities/str_util.h"
#include "utilities/tree_copy.h"

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>

#define PATH_SIZE 1200

struct AccountExportManager {
    AccountExportManagerDeps deps;
};

AccountExportManager *account_export_manager_create(const AccountExportManagerDeps *deps) {
    AccountExportManager *m = calloc(1, sizeof(*m));
    if (m) m->deps = *deps;
    return m;
}

void account_export_manager_destroy(AccountExportManager *m) { free(m); }

int account_export_manager_available(AccountExportManager *m) { return m->deps.cipher->available(m->deps.cipher); }

static int refuse(char *why, unsigned long size, const char *text) {
    str_copy(why, size, text);
    return -1;
}

/* The account a person named: by its label, or by its id. */
static int find_account(AccountExportManager *m, const char *ref, Account *out) {
    Account all[ACCOUNT_MAX];
    int n = m->deps.accounts->list(m->deps.accounts, all, ACCOUNT_MAX);
    char *end = NULL;
    long id = ref ? strtol(ref, &end, 10) : 0;
    int numeric = ref && *ref && end && *end == '\0';
    for (int i = 0; i < n; i++) {
        if ((numeric && all[i].id == (AccountId)id) || (ref && account_label_same(all[i].label, ref))) {
            *out = all[i];
            return 0;
        }
    }
    return -1;
}

typedef struct MediaCopy {
    const char *media_dir;
    const char *target_dir;       /* content/media */
    MigrationReport *report;
} MediaCopy;

static void make_parents(const char *file) {
    char dir[PATH_SIZE];
    str_copy(dir, sizeof(dir), file);
    char *slash = strrchr(dir, '/');
    if (slash) { *slash = '\0'; path_mkdir_p(dir, 0700); }
}

static void copy_one_media(void *ctx, const char *path) {
    MediaCopy *c = ctx;
    char rel[PATH_SIZE], dest[PATH_SIZE];
    if (media_path_relative(c->media_dir, path, rel, sizeof(rel)) != 0) return;     /* not in the media folder */
    if (!path_is_regular_file(path)) { c->report->media_missing++; return; }
    int n = snprintf(dest, sizeof(dest), "%s/%s", c->target_dir, rel);
    if (n < 0 || (size_t)n >= sizeof(dest)) return;
    make_parents(dest);
    if (file_copy(path, dest, 0600) == 0) c->report->media_files++;
}

static int write_manifest(const char *dir, const AccountPackageManifest *manifest) {
    char text[1500], path[PATH_SIZE];
    if (account_package_manifest_format(manifest, text, sizeof(text)) != 0) return -1;
    snprintf(path, sizeof(path), "%s/account-manifest.txt", dir);
    int fd = open(path, O_WRONLY | O_CREAT | O_EXCL | O_CLOEXEC | O_NOFOLLOW, 0600);
    if (fd < 0) return -1;
    ssize_t want = (ssize_t)strlen(text);
    int ok = write(fd, text, (size_t)want) == want;
    close(fd);
    return ok ? 0 : -1;
}

int account_export_manager_export(AccountExportManager *m, const AccountExportRequest *r, const Passphrase *passphrase,
                                  MigrationReport *report, char *why, unsigned long size) {
    memset(report, 0, sizeof(*report));
    if (!account_export_manager_available(m)) return refuse(why, size, "openssl is not installed, so the package cannot be encrypted");
    Account account;
    if (find_account(m, r->account, &account) != 0) return refuse(why, size, "there is no such account (use its label, as the accounts dialog shows it)");

    /* Claim the output first: never replace or write through anything already there. */
    int out = open(r->output, O_WRONLY | O_CREAT | O_EXCL | O_CLOEXEC | O_NOFOLLOW, 0600);
    if (out < 0) return refuse(why, size, errno == EEXIST ? "that file already exists" : "that file cannot be created");
    close(out);

    char staging[PATH_SIZE], content[PATH_SIZE], package[PATH_SIZE], media[PATH_SIZE], archive[PATH_SIZE];
    if (staging_dir_make(r->paths.data_dir, "migrate", staging, sizeof(staging)) != 0) {
        unlink(r->output);
        return refuse(why, size, "cannot make a working folder in the data folder");
    }
    snprintf(content, sizeof(content), "%s/content", staging);
    snprintf(package, sizeof(package), "%s/package.db", content);
    snprintf(media, sizeof(media), "%s/media", content);
    snprintf(archive, sizeof(archive), "%s/package.tar.gz", staging);

    AccountPackageManifest manifest;
    memset(&manifest, 0, sizeof(manifest));
    manifest.format = ACCOUNT_PACKAGE_FORMAT;
    str_copy(manifest.app_version, sizeof(manifest.app_version), APP_VERSION);
    manifest.schema_version = m->deps.schema->version(m->deps.schema, NULL);
    manifest.created = (int64_t)time(NULL);
    manifest.account_id = account.id;
    str_copy(manifest.label, sizeof(manifest.label), account.label);
    str_copy(manifest.jid, sizeof(manifest.jid), account.jid);
    str_copy(manifest.name, sizeof(manifest.name), account.name);
    manifest.colour = account.colour;
    str_copy(manifest.media_dir, sizeof(manifest.media_dir), r->paths.media_dir ? r->paths.media_dir : "");

    int rc = -1;
    const char *names[3];
    int count = 0;
    if (mkdir(content, 0700) != 0) {
        refuse(why, size, "cannot make a working folder in the data folder");
    } else if (m->deps.extractor->extract(m->deps.extractor, account.id, package, report) != 0) {
        refuse(why, size, "could not read that account's chats from the database");
    } else {
        names[count++] = "package.db";
        if (r->with_media && r->paths.media_dir) {
            mkdir(media, 0700);
            MediaCopy copy = { r->paths.media_dir, media, report };
            m->deps.extractor->media_paths(m->deps.extractor, package, copy_one_media, &copy);
            if (report->media_files > 0) { manifest.has_media = 1; names[count++] = "media"; }
        }
        names[count++] = "account-manifest.txt";
        if (write_manifest(content, &manifest) != 0) {
            refuse(why, size, "could not write the package's description");
        } else if (m->deps.archive->create(m->deps.archive, content, names, count, archive) != 0) {
            refuse(why, size, "tar could not pack the package");
        } else if (m->deps.cipher->encrypt(m->deps.cipher, archive, r->output, passphrase) != 0) {
            refuse(why, size, "openssl could not encrypt the package");
        } else {
            chmod(r->output, 0600);
            rc = 0;
            LOG_INFO("account package written (%d messages, %d media files)", report->messages, report->media_files);
        }
    }
    tree_remove(staging);
    if (rc != 0) unlink(r->output);
    return rc;
}
