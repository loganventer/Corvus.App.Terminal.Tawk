#ifndef APP_ENGINES_LOCK_PASSWORD_RULE_H
#define APP_ENGINES_LOCK_PASSWORD_RULE_H

#include "core/lock_password_problem.h"
#include "core/passphrase.h"

#define LOCK_PASSWORD_MIN_CHARS 4

/* Whether a new lock password, typed twice, may be used. */
LockPasswordProblem lock_password_rule_check(const Passphrase *password, const Passphrase *confirm);
/* What the dialog says about a problem; "" for LOCK_PASSWORD_OK. */
const char *lock_password_problem_text(LockPasswordProblem problem);

#endif
