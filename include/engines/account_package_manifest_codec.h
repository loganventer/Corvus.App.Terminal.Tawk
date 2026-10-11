#ifndef APP_ENGINES_ACCOUNT_PACKAGE_MANIFEST_CODEC_H
#define APP_ENGINES_ACCOUNT_PACKAGE_MANIFEST_CODEC_H

#include <stddef.h>

#include "core/account_package_manifest.h"

/* account-manifest.txt: "key=value" lines, readable by people. */
int account_package_manifest_format(const AccountPackageManifest *manifest, char *out, size_t size);
/* Returns 0 when `text` is a manifest of a format this version knows; unknown keys are ignored. */
int account_package_manifest_parse(const char *text, AccountPackageManifest *out);

#endif
