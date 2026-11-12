#include "engines/lock_password_rule.h"

#include <stddef.h>

static int characters(const Passphrase *p) {
    int count = 0;
    for (size_t i = 0; i < p->length; i++) if (((unsigned char)p->text[i] & 0xC0) != 0x80) count++;
    return count;
}

LockPasswordProblem lock_password_rule_check(const Passphrase *password, const Passphrase *confirm) {
    if (!password || password->length == 0) return LOCK_PASSWORD_EMPTY;
    if (password->length > PASSPHRASE_MAX) return LOCK_PASSWORD_TOO_LONG;
    if (characters(password) < LOCK_PASSWORD_MIN_CHARS) return LOCK_PASSWORD_TOO_SHORT;
    if (!confirm || !passphrase_equal(password, confirm)) return LOCK_PASSWORD_MISMATCH;
    return LOCK_PASSWORD_OK;
}

const char *lock_password_problem_text(LockPasswordProblem problem) {
    switch (problem) {
    case LOCK_PASSWORD_OK:        return "";
    case LOCK_PASSWORD_EMPTY:     return "Type a password";
    case LOCK_PASSWORD_TOO_SHORT: return "Use at least 4 characters";
    case LOCK_PASSWORD_TOO_LONG:  return "That is too long; use at most 256 bytes";
    case LOCK_PASSWORD_MISMATCH:  return "The two passwords are not the same";
    case LOCK_PASSWORD_NOT_SAVED: return "The password could not be saved; see the log";
    }
    return "";
}
