#ifndef APP_RESOURCE_ACCESS_GATEWAY_OPTIONS_H
#define APP_RESOURCE_ACCESS_GATEWAY_OPTIONS_H

/* Construction parameters shared by every gateway implementation. */
typedef struct GatewayOptions {
    char auth_dir[512];
    char media_dir[512];
    char sidecar_dir[512];
    char node_binary[256];
    char log_path[512];     /* backend diagnostics file (sidecar stderr) */
    char log_dir[512];      /* folder for backend logs (XDG state) */
    int  debug;
} GatewayOptions;

#endif
