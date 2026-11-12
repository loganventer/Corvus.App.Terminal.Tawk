#ifndef APP_CORE_LOCK_PASSWORD_PROBLEM_H
#define APP_CORE_LOCK_PASSWORD_PROBLEM_H

/* What is wrong with a new lock password, if anything. */
typedef enum {
    LOCK_PASSWORD_OK = 0,
    LOCK_PASSWORD_EMPTY,       /* nothing was typed */
    LOCK_PASSWORD_TOO_SHORT,   /* fewer than four characters */
    LOCK_PASSWORD_TOO_LONG,    /* more than PASSPHRASE_MAX bytes */
    LOCK_PASSWORD_MISMATCH     /* the confirmation differs */
} LockPasswordProblem;

#endif
