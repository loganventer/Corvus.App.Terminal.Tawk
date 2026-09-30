#ifndef APP_MANAGERS_PROFILE_MANAGER_DEPS_H
#define APP_MANAGERS_PROFILE_MANAGER_DEPS_H

#include "contracts/i_message_gateway.h"
#include "contracts/i_profile_store.h"

typedef struct ProfileManagerDeps {
    IMessageGateway *gateway;
    IProfileStore   *store;
    const char      *media_dir;   /* picture files must be inside it */
} ProfileManagerDeps;

#endif
