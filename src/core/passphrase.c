#include "core/passphrase.h"
#include "utilities/secure_zero.h"

#include <string.h>

int passphrase_set(Passphrase *p, const char *text) {
    passphrase_wipe(p);
    size_t n = text ? strlen(text) : 0;
    if (n == 0 || n > PASSPHRASE_MAX) return -1;
    memcpy(p->text, text, n);
    p->text[n] = '\0';
    p->length = n;
    return 0;
}

int passphrase_equal(const Passphrase *a, const Passphrase *b) {
    if (a->length != b->length) return 0;
    unsigned char diff = 0;                           /* no early exit on the first difference */
    for (size_t i = 0; i < a->length; i++) diff |= (unsigned char)(a->text[i] ^ b->text[i]);
    return diff == 0;
}

void passphrase_wipe(Passphrase *p) {
    secure_zero(p, sizeof(*p));
}
