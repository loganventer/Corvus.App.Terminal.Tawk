#ifndef APP_ENGINES_ACCOUNT_IMPORT_POLICY_H
#define APP_ENGINES_ACCOUNT_IMPORT_POLICY_H

#include <stddef.h>

#include "core/account.h"
#include "core/account_package_manifest.h"
#include "core/import_verdict.h"

/* Whether this tawk may bring the package in: a format it knows, made by this schema or an older one, with
 * room for one more account. */
ImportVerdict account_import_policy_check(const AccountPackageManifest *manifest, int local_schema_version,
                                          int existing_accounts);
/* A label for the account here: `wanted` when it is a good label nobody has, otherwise a free variation of
 * it ("Personal 2"), and "Moved" when there is nothing to start from. Returns 0 and fills `out`. */
int account_import_policy_label(const char *wanted, const Account *existing, int count, char *out, size_t size);

#endif
