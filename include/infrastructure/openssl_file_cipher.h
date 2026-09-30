#ifndef APP_INFRASTRUCTURE_OPENSSL_FILE_CIPHER_H
#define APP_INFRASTRUCTURE_OPENSSL_FILE_CIPHER_H

#include "contracts/i_file_cipher.h"

/* Encrypts files with the openssl tool: AES-256 with a key derived from the
 * passphrase by PBKDF2 (600,000 rounds) and a random salt. The passphrase
 * reaches openssl through a pipe (-pass fd:3), never its arguments. */
IFileCipher *openssl_file_cipher_create(void);

#endif
