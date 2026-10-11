#ifndef APP_MANAGERS_ACCOUNT_IMPORT_MANAGER_DEPS_H
#define APP_MANAGERS_ACCOUNT_IMPORT_MANAGER_DEPS_H

#include "contracts/i_account_extractor.h"
#include "contracts/i_account_link_expectation.h"
#include "contracts/i_account_merger.h"
#include "contracts/i_account_store.h"
#include "contracts/i_archive.h"
#include "contracts/i_file_cipher.h"
#include "contracts/i_schema_inspector.h"

/* What the account import manager depends on, injected by the composition root. */
typedef struct AccountImportManagerDeps {
    IAccountStore           *accounts;
    IAccountMerger          *merger;
    IAccountExtractor       *extractor;      /* only to list the media paths a package names */
    IAccountLinkExpectation *expectation;
    ISchemaInspector        *schema;
    IArchive                *archive;
    IFileCipher             *cipher;
} AccountImportManagerDeps;

#endif
