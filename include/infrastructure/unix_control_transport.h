#ifndef APP_INFRASTRUCTURE_UNIX_CONTROL_TRANSPORT_H
#define APP_INFRASTRUCTURE_UNIX_CONTROL_TRANSPORT_H

#include "contracts/i_control_transport.h"

/* The control socket as a Unix domain socket: 0600 in a 0700 folder,
 * clients of the same user only, lines of at most 1 MiB. */
IControlTransport *unix_control_transport_create(void);

#endif
