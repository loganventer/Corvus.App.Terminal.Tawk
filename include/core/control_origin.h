#ifndef APP_CORE_CONTROL_ORIGIN_H
#define APP_CORE_CONTROL_ORIGIN_H

/* Who a control socket client says it acts for: a program working for a
 * language model, or a person's own shell command. */
typedef enum ControlOrigin {
    CONTROL_ORIGIN_MCP = 0,
    CONTROL_ORIGIN_CLI
} ControlOrigin;

/* "mcp" or "cli"; returns -1 for anything else. */
int         control_origin_parse(const char *name, ControlOrigin *out);
const char *control_origin_name(ControlOrigin origin);

#endif
