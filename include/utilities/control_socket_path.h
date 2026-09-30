#ifndef APP_UTILITIES_CONTROL_SOCKET_PATH_H
#define APP_UTILITIES_CONTROL_SOCKET_PATH_H

#include <stddef.h>

/* Where the control socket lives: $TAWK_CONTROL_SOCKET when set, else
 * control.sock in the runtime folder (see path_runtime_dir). */
void control_socket_path(char *out, size_t size);

#endif
