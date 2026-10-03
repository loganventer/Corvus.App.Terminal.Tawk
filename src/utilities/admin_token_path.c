#include "utilities/admin_token_path.h"
#include "utilities/control_socket_path.h"

#include <stdio.h>
#include <string.h>

void admin_token_path(char *out, size_t size) {
    char socket[600];
    control_socket_path(socket, sizeof(socket));
    char *slash = strrchr(socket, '/');
    if (slash) *slash = '\0';
    snprintf(out, size, "%s/admin.token", slash ? socket : ".");
}
