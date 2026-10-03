#ifndef APP_RESOURCE_ACCESS_FILE_ADMIN_TOKEN_STORE_H
#define APP_RESOURCE_ACCESS_FILE_ADMIN_TOKEN_STORE_H

#include "contracts/i_admin_token_store.h"

/* Keeps the admin token in the file at `path`, mode 0600. */
IAdminTokenStore *file_admin_token_store_create(const char *path);

#endif
