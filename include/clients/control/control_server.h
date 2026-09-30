#ifndef APP_CLIENTS_CONTROL_CONTROL_SERVER_H
#define APP_CLIENTS_CONTROL_CONTROL_SERVER_H

#include "clients/control/control_server_deps.h"
#include "contracts/i_frame_hook.h"

/* The control client: a second way into tawk beside the terminal, for
 * tawk-mcp and the tawk send, tail and unread commands. It speaks the
 * protocol in CONTROL.md over the control socket and uses the same
 * managers as the terminal client, asking the automation manager what is
 * allowed. It listens only while Settings > Automation > Control socket is
 * on, and runs on the UI thread through its frame hook. */
typedef struct ControlServer ControlServer;

ControlServer *control_server_create(const ControlServerDeps *deps);
/* Says goodbye to connected clients and stops listening. */
void           control_server_destroy(ControlServer *server);
/* Handles everything that arrived; call once a frame. Returns 1 when
 * something it did may change what is shown. */
int            control_server_tick(ControlServer *server);
IFrameHook    *control_server_frame_hook(ControlServer *server);

#endif
