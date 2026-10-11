#ifndef APP_MANAGERS_ACCOUNT_EXPORT_MANAGER_DEPS_H
#define APP_MANAGERS_ACCOUNT_EXPORT_MANAGER_DEPS_H

#include "contracts/i_account_extractor.h"
#include "contracts/i_account_store.h"
#include "contracts/i_archive.h"
#include "contracts/i_file_cipher.h"
#include "contracts/i_schema_inspector.h"

/* What the account export manager depends on, injected by the composition root. */
typedef struct AccountExportManagerDeps {
    IAccountStore     *accounts;
    IAccountExtractor *extractor;
    ISchemaInspector  *schema;
    IArchive          *archive;
    IFileCipher       *cipher;
} AccountExportManagerDeps;

#endif
