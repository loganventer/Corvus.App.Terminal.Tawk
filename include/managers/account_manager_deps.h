#ifndef APP_MANAGERS_ACCOUNT_MANAGER_DEPS_H
#define APP_MANAGERS_ACCOUNT_MANAGER_DEPS_H

#include "contracts/i_profile_editor.h"

/* What the account manager depends on, injected by the composition root. */
typedef struct AccountManagerDeps {
    IProfileEditor *editor;
    const char     *media_dir;
} AccountManagerDeps;

#endif
