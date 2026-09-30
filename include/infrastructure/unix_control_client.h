#ifndef APP_INFRASTRUCTURE_UNIX_CONTROL_CLIENT_H
#define APP_INFRASTRUCTURE_UNIX_CONTROL_CLIENT_H

#include "contracts/i_control_client.h"

/* Connects to a running tawk's control socket, for shell commands. */
IControlClient *unix_control_client_create(void);

#endif
