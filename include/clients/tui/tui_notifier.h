#ifndef APP_CLIENTS_TUI_TUI_NOTIFIER_H
#define APP_CLIENTS_TUI_TUI_NOTIFIER_H

#include "clients/tui/blink_state.h"
#include "clients/tui/title_flasher.h"
#include "contracts/i_notifier.h"
#include "contracts/i_terminal_title.h"
#include "core/settings.h"

/* In-terminal alerts: blinks the chat, flashes the window title, and rings
 * the bell or flashes the screen when enabled. Borrows all arguments. */
INotifier *tui_notifier_create(BlinkState *blink, TitleFlasher *flasher, ITerminalTitle *title,
                               const Settings *settings);

#endif
