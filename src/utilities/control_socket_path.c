#include "utilities/control_socket_path.h"
#include "utilities/path_util.h"
#include "utilities/str_util.h"

#include <stdlib.h>

void control_socket_path(char *out, size_t size) {
    const char *forced = getenv("TAWK_CONTROL_SOCKET");
    if (forced && forced[0] == '/') { str_copy(out, size, forced); return; }
    char dir[512];
    path_runtime_dir(dir, sizeof(dir));
    path_join(out, size, dir, "control.sock");
}
