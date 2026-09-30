#include "core/control_origin.h"

#include <string.h>

int control_origin_parse(const char *name, ControlOrigin *out) {
    if (!name) return -1;
    if (strcmp(name, "mcp") == 0) { *out = CONTROL_ORIGIN_MCP; return 0; }
    if (strcmp(name, "cli") == 0) { *out = CONTROL_ORIGIN_CLI; return 0; }
    return -1;
}

const char *control_origin_name(ControlOrigin origin) { return origin == CONTROL_ORIGIN_CLI ? "cli" : "mcp"; }
