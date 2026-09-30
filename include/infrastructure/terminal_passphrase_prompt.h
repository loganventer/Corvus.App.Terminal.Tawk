#ifndef APP_INFRASTRUCTURE_TERMINAL_PASSPHRASE_PROMPT_H
#define APP_INFRASTRUCTURE_TERMINAL_PASSPHRASE_PROMPT_H

#include "contracts/i_passphrase_prompt.h"

/* Reads passphrases from the controlling terminal (/dev/tty) with echo
 * turned off, restoring the terminal even when interrupted. Used before the
 * full-screen interface starts, and by the command-line tools. */
IPassphrasePrompt *terminal_passphrase_prompt_create(void);

#endif
