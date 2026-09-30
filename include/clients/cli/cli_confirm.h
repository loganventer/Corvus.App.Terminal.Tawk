#ifndef APP_CLIENTS_CLI_CLI_CONFIRM_H
#define APP_CLIENTS_CLI_CLI_CONFIRM_H

/* Asks a yes-or-no question on the terminal. `assume_yes` (--yes) answers
 * yes without asking; otherwise the answer is no unless the person types y.
 * Returns 1 for yes. */
int cli_confirm(const char *question, int assume_yes);

#endif
