#ifndef APP_CLIENTS_CLI_UPDATER_H
#define APP_CLIENTS_CLI_UPDATER_H

#include "clients/cli/updater_options.h"

/* `tawk --update`: compares this build's commit with the newest on GitHub
 * and stops when they match. Otherwise (or always, with --reinstall) it
 * asks, then downloads install.sh to a private temporary file and runs it
 * with the same install prefix and backends. Returns the exit status for
 * main(). */
int updater_run(const UpdaterOptions *options);

#endif
