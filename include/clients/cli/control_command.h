#ifndef APP_CLIENTS_CLI_CONTROL_COMMAND_H
#define APP_CLIENTS_CLI_CONTROL_COMMAND_H

#include "clients/cli/control_options.h"
#include "contracts/i_control_client.h"

#define CONTROL_EXIT_OK          0
#define CONTROL_EXIT_FAILED      1
#define CONTROL_EXIT_REFUSED     2   /* declined, not allowed or not answered in time */
#define CONTROL_EXIT_NOT_RUNNING 3

/* Runs one shell command against the tawk listening at `socket_path`
 * (see CONTROL.md); returns one of the exit codes above. */
int control_command_run(const ControlOptions *options, IControlClient *client, const char *socket_path);

#endif
