#ifndef APP_CORE_PASSPHRASE_H
#define APP_CORE_PASSPHRASE_H

#include <stddef.h>

#define PASSPHRASE_MAX 256

/* A passphrase typed by the user, for the database or a backup. It lives
 * only in memory for as long as it is needed and is wiped after use; it is
 * never a setting, an argument or an environment variable. */
typedef struct Passphrase {
    char   text[PASSPHRASE_MAX + 1];
    size_t length;
} Passphrase;

/* Copies `text` in; returns -1 when it is empty or too long. */
int  passphrase_set(Passphrase *passphrase, const char *text);
int  passphrase_equal(const Passphrase *a, const Passphrase *b);
/* Overwrites it so the text does not linger in memory. */
void passphrase_wipe(Passphrase *passphrase);

#endif
