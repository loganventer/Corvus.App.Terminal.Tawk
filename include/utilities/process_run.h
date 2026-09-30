#ifndef APP_UTILITIES_PROCESS_RUN_H
#define APP_UTILITIES_PROCESS_RUN_H

/* Runs argv (no shell) in the foreground with the terminal's own stdin,
 * stdout and stderr, and waits for it. Returns its exit status, or -1 when
 * it could not be started. For command-line tools, never the TUI. */
int process_run_foreground(char *const argv[]);

#endif
