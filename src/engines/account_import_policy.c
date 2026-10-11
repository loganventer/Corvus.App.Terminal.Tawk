#include "engines/account_import_policy.h"
#include "engines/account_label_validator.h"
#include "utilities/str_util.h"

#include <stdio.h>
#include <string.h>

ImportVerdict account_import_policy_check(const AccountPackageManifest *m, int local_schema, int existing) {
    if (!m || m->format < 1 || m->format > ACCOUNT_PACKAGE_FORMAT) return IMPORT_BAD_MANIFEST;
    if (m->schema_version > local_schema) return IMPORT_NEWER_SCHEMA;
    if (existing >= ACCOUNT_MAX) return IMPORT_NO_ROOM;
    return IMPORT_OK;
}

static int taken(const char *label, const Account *existing, int count) {
    for (int i = 0; i < count; i++) {
        if (account_label_same(label, existing[i].label)) return 1;
    }
    return 0;
}

/* Cuts `text` to at most `bytes`, never in the middle of a character. */
static void cut_to(char *text, size_t bytes) {
    if (strlen(text) <= bytes) return;
    while (bytes > 0 && ((unsigned char)text[bytes] & 0xC0) == 0x80) bytes--;
    text[bytes] = '\0';
}

int account_import_policy_label(const char *wanted, const Account *existing, int count, char *out, size_t size) {
    char why[64], base[ACCOUNT_LABEL_SIZE];
    str_copy(base, sizeof(base), wanted ? wanted : "");
    if (account_label_validate(base, why, sizeof(why)) != 0) str_copy(base, sizeof(base), "Moved");
    if (!taken(base, existing, count)) { str_copy(out, size, base); return 0; }
    for (int n = 2; n < 100; n++) {
        char suffix[8], candidate[ACCOUNT_LABEL_SIZE];
        snprintf(suffix, sizeof(suffix), " %d", n);
        str_copy(candidate, sizeof(candidate), base);
        /* 24 characters at most: cut the base in bytes, which is never more than the characters it holds. */
        cut_to(candidate, ACCOUNT_LABEL_MAX_CHARS - strlen(suffix));
        strncat(candidate, suffix, sizeof(candidate) - strlen(candidate) - 1);
        if (account_label_validate(candidate, why, sizeof(why)) == 0 && !taken(candidate, existing, count)) {
            str_copy(out, size, candidate);
            return 0;
        }
    }
    return -1;
}
