#ifndef APP_CLIENTS_CLI_UPDATER_OPTIONS_H
#define APP_CLIENTS_CLI_UPDATER_OPTIONS_H

/* How `tawk --update` was asked to run. */
typedef struct UpdaterOptions {
    int         assume_yes;       /* --yes: do not ask before installing */
    int         reinstall;        /* --reinstall: install even when up to date */
    const char *self_path;        /* this binary (from /proc/self/exe or argv[0]) */
    const char *sidecar_dir;      /* the Node.js backend folder, installed or not */
    int         whatsmeow_built;
} UpdaterOptions;

#endif
