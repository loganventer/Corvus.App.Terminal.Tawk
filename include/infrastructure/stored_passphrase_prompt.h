#ifndef APP_INFRASTRUCTURE_STORED_PASSPHRASE_PROMPT_H
#define APP_INFRASTRUCTURE_STORED_PASSPHRASE_PROMPT_H

#include "contracts/i_passphrase_prompt.h"

/* A passphrase prompt that first tries a key file the user chose to keep,
 * and asks at the terminal when there is none. The file holds the passphrase
 * as one line of text; it is used only when it is a plain file owned by the
 * user and unreadable by anyone else (mode 0600), and it is never followed
 * through a link.
 *
 * It answers from the file once, for the first plain question (not a new
 * passphrase), so a stale file costs one try and never locks the user out.
 * `fallback` is not owned and must outlive the prompt. `path` may be NULL. */
IPassphrasePrompt *stored_passphrase_prompt_create(IPassphrasePrompt *fallback, const char *path);

/* The same, with the file beside the settings: `db.key` in the folder of
 * `config_path`, such as ~/.config/corvus/tawk/db.key. */
IPassphrasePrompt *stored_passphrase_prompt_create_default(IPassphrasePrompt *fallback, const char *config_path);

#endif
