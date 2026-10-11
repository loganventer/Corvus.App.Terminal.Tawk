#include "clients/cli/migrate_command.h"
#include "clients/cli/cli_confirm.h"
#include "utilities/app_info.h"
#include "utilities/staging_dir.h"
#include "utilities/str_util.h"
#include "utilities/tree_copy.h"

#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>

static void line(const char *what, int n, const char *unit) {
    printf("  %-22s %d %s\n", what, n, unit);
}

int migrate_out_command_run(AccountExportManager *mgr, IPassphrasePrompt *prompt, const AccountExportRequest *r) {
    if (!account_export_manager_available(mgr)) {
        fprintf(stderr, "%s: packages are encrypted with openssl, which is not installed\n", APP_NAME);
        return 1;
    }
    printf("Packing the account \"%s\" into %s.\n"
           "It holds the account's chats, messages, contacts, statuses and the choices you made for its chats.\n"
           "It does not hold the WhatsApp login: the other computer links by QR code, with your phone.\n"
           "Nothing on this computer changes.\n", r->account, r->output);
    printf("\nThe package is encrypted with a passphrase. You need it on the other computer; it cannot be recovered.\n");
    Passphrase passphrase;
    if (prompt->ask(prompt, "Passphrase for the package: ", 1, &passphrase) != 0) {
        fprintf(stderr, "%s: no passphrase given; nothing was written\n", APP_NAME);
        return 1;
    }
    MigrationReport report;
    char why[512];
    int rc = account_export_manager_export(mgr, r, &passphrase, &report, why, sizeof(why));
    passphrase_wipe(&passphrase);
    if (rc != 0) {
        fprintf(stderr, "%s: no package was made: %s\n", APP_NAME, why);
        return 1;
    }
    struct stat st;
    double mb = stat(r->output, &st) == 0 ? (double)st.st_size / (1024.0 * 1024.0) : 0;
    printf("Package written: %s (%.1f MB)\n", r->output, mb);
    line("messages", report.messages, "");
    line("chats", report.chats, "");
    line("contacts", report.contacts, "");
    line("other records", report.other_rows, "");
    if (r->with_media) {
        line("media files", report.media_files, "");
        if (report.media_missing > 0) line("media no longer on disk", report.media_missing, "");
    } else {
        printf("  media                  left out (fetched again from WhatsApp when you open a message)\n");
    }
    if (report.scheduled_left_behind > 0) {
        printf("  %d message%s waiting for a time stay here, so they are not sent twice.\n", report.scheduled_left_behind,
               report.scheduled_left_behind == 1 ? "" : "s");
    }
    printf("\nOn the other computer: %s --migrate-in %s\n", APP_NAME, r->output);
    return 0;
}

int migrate_in_command_run(AccountImportManager *mgr, IPassphrasePrompt *prompt, const AccountImportRequest *r, int assume_yes) {
    if (!account_import_manager_available(mgr)) {
        fprintf(stderr, "%s: packages are decrypted with openssl, which is not installed\n", APP_NAME);
        return 1;
    }
    printf("Bringing %s in adds one account to this tawk. Your other accounts are not changed, and a copy of\n"
           "the database as it is now is kept beside it.\n", r->input);
    if (!cli_confirm("Bring it in now?", assume_yes)) {
        printf("Nothing was changed.\n");
        return 1;
    }
    Passphrase passphrase;
    if (prompt->ask(prompt, "Passphrase of the package: ", 0, &passphrase) != 0) {
        fprintf(stderr, "%s: no passphrase given; nothing was changed\n", APP_NAME);
        return 1;
    }
    MigrationReport report;
    Account account;
    char why[1300];
    memset(&account, 0, sizeof(account));
    int rc = account_import_manager_import(mgr, r, &passphrase, &report, &account, why, sizeof(why));
    passphrase_wipe(&passphrase);
    if (rc != 0) {
        fprintf(stderr, "%s: %s\n", APP_NAME, why);
        return 1;
    }
    printf("Added the account \"%s\":\n", account.label);
    line("messages", report.messages, "");
    line("chats", report.chats, "");
    line("contacts", report.contacts, "");
    line("other records", report.other_rows, "");
    printf("\nStart %s and scan its QR code with the phone that has this WhatsApp number.\n"
           "Agents have no access to the account until you give it in the accounts dialog.\n", APP_NAME);
    return 0;
}

int migrate_keep_database_copy(IDatabaseSnapshot *snapshot, const char *data_dir, const char *db_path, char *kept, unsigned long size) {
    kept[0] = '\0';
    if (access(db_path, F_OK) != 0) return 0;
    char staging[1200], taken[1300], suffix[48];
    if (staging_dir_make(data_dir, "migrate-copy", staging, sizeof(staging)) != 0) return -1;
    int rc = snapshot->take(snapshot, db_path, staging);
    if (rc == 0) {
        time_t now = time(NULL);
        struct tm tm_now;
        localtime_r(&now, &tm_now);
        strftime(suffix, sizeof(suffix), ".before-migrate-%Y%m%d-%H%M%S", &tm_now);
        snprintf(taken, sizeof(taken), "%s/tawk.db", staging);
        snprintf(kept, size, "%s%s", db_path, suffix);
        rc = rename(taken, kept) == 0 ? 0 : -1;
        if (rc != 0) kept[0] = '\0';
    }
    tree_remove(staging);
    return rc;
}
