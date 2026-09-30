#ifndef APP_CLIENTS_TUI_TUI_KEY_NEWLINE_H
#define APP_CLIENTS_TUI_TUI_KEY_NEWLINE_H

/* Shift+Enter or Alt+Enter: a line break in the text being typed, where
 * Enter alone sends or saves. Passed on as a key code (is_key set) above
 * anything curses or terminfo uses. */
#define TUI_KEY_NEWLINE 0x7E000

#endif
