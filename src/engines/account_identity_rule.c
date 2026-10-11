#include "engines/account_identity_rule.h"

#include <string.h>

/* The person part of a JID: what comes before ':' or '@'. */
static size_t person_length(const char *jid) {
    size_t n = strcspn(jid, ":@");
    return n;
}

IdentityVerdict account_identity_rule(const char *expected, const char *linked) {
    if (!expected || !*expected) return IDENTITY_NOT_EXPECTED;
    if (!linked || !*linked) return IDENTITY_MISMATCH;
    size_t a = person_length(expected), b = person_length(linked);
    return a > 0 && a == b && strncmp(expected, linked, a) == 0 ? IDENTITY_MATCH : IDENTITY_MISMATCH;
}
