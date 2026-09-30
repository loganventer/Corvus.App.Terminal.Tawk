#ifndef APP_MANAGERS_DATABASE_CRYPT_MANAGER_DEPS_H
#define APP_MANAGERS_DATABASE_CRYPT_MANAGER_DEPS_H

#include "contracts/i_database_cipher.h"

/* What the database encryption manager depends on, injected by the composition root. */
typedef struct DatabaseCryptManagerDeps {
    IDatabaseCipher *cipher;
    const char      *db_path;
} DatabaseCryptManagerDeps;

#endif
