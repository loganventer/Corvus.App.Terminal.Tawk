#include "infrastructure/openssl_file_cipher.h"
#include "utilities/process_quiet.h"
#include "utilities/process_util.h"

#include <stdlib.h>
#include <unistd.h>

#define PASSPHRASE_FD 3

static int run(const char *in, const char *out, const Passphrase *p, int decrypt) {
    char *argv[16];
    int n = 0;
    argv[n++] = "openssl";
    argv[n++] = "enc";
    if (decrypt) argv[n++] = "-d";
    argv[n++] = "-aes-256-cbc";
    argv[n++] = "-pbkdf2";
    argv[n++] = "-iter";
    argv[n++] = "600000";
    argv[n++] = "-salt";
    argv[n++] = "-in";
    argv[n++] = (char *)in;
    argv[n++] = "-out";
    argv[n++] = (char *)out;
    argv[n++] = "-pass";
    argv[n++] = "fd:3";
    argv[n] = NULL;
    int rc = process_run_quiet(argv, p->text, p->length, PASSPHRASE_FD);
    if (rc != 0) unlink(out);                                /* never leave half a file */
    return rc == 0 ? 0 : -1;
}

static int cipher_encrypt(IFileCipher *self, const char *in, const char *out, const Passphrase *p) { (void)self; return run(in, out, p, 0); }
static int cipher_decrypt(IFileCipher *self, const char *in, const char *out, const Passphrase *p) { (void)self; return run(in, out, p, 1); }
static int available(IFileCipher *self) { (void)self; return process_on_path("openssl"); }
static void destroy(IFileCipher *self) { free(self); }

IFileCipher *openssl_file_cipher_create(void) {
    IFileCipher *c = calloc(1, sizeof(*c));
    if (!c) return NULL;
    c->encrypt = cipher_encrypt;   /* not "encrypt": macOS declares one in <unistd.h> */
    c->decrypt = cipher_decrypt;
    c->available = available;
    c->destroy = destroy;
    return c;
}
