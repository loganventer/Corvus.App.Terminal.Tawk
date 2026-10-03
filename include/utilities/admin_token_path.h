#ifndef APP_UTILITIES_ADMIN_TOKEN_PATH_H
#define APP_UTILITIES_ADMIN_TOKEN_PATH_H

#include <stddef.h>

/* Where the admin token is kept: admin.token beside the control socket
 * (see control_socket_path). */
void admin_token_path(char *out, size_t size);

#endif
