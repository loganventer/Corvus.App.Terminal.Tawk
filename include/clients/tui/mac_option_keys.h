#ifndef APP_CLIENTS_TUI_MAC_OPTION_KEYS_H
#define APP_CLIENTS_TUI_MAC_OPTION_KEYS_H

/* On a Mac, Option+letter types a character (Option+L is ¬) unless the
 * terminal sends Option as Alt. This gives the letter of tawk's Alt
 * shortcut such a character stands for (US layout), or 0. Symbols nobody
 * types in a message (¬ √ ® ´) always count; letters used in some
 * languages (å ø µ π œ) only when `typing_text` is false. */
int mac_option_letter(int ch, int typing_text);

#endif
