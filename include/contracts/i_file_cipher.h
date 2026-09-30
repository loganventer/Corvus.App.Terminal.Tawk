#ifndef APP_CONTRACTS_I_FILE_CIPHER_H
#define APP_CONTRACTS_I_FILE_CIPHER_H

#include "core/passphrase.h"

/* Encrypts and decrypts whole files with a passphrase. */
typedef struct IFileCipher {
    void *ctx;
    /* Writes the encrypted `in` to `out`. Returns 0 on success. */
    int  (*encrypt)(struct IFileCipher *self, const char *in, const char *out, const Passphrase *passphrase);
    /* Returns 0 on success, -1 with a wrong passphrase or a file it did not make. */
    int  (*decrypt)(struct IFileCipher *self, const char *in, const char *out, const Passphrase *passphrase);
    /* True when the tool it needs is installed. */
    int  (*available)(struct IFileCipher *self);
    void (*destroy)(struct IFileCipher *self);
} IFileCipher;

#endif
