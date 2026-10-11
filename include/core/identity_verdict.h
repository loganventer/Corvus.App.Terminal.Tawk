#ifndef APP_CORE_IDENTITY_VERDICT_H
#define APP_CORE_IDENTITY_VERDICT_H

/* Whether the number that just linked is the one a moved account came from. */
typedef enum IdentityVerdict {
    IDENTITY_NOT_EXPECTED = 0,  /* the account was not moved here; nothing to check */
    IDENTITY_MATCH,
    IDENTITY_MISMATCH
} IdentityVerdict;

#endif
