#ifndef APP_CLIENTS_CLI_DATABASE_CRYPT_ACTION_H
#define APP_CLIENTS_CLI_DATABASE_CRYPT_ACTION_H

/* What `tawk --encrypt`, `--decrypt` or `--change-passphrase` asked for. */
typedef enum DatabaseCryptAction {
    DATABASE_CRYPT_NONE = 0,
    DATABASE_CRYPT_ENCRYPT,
    DATABASE_CRYPT_DECRYPT,
    DATABASE_CRYPT_CHANGE
} DatabaseCryptAction;

#endif
