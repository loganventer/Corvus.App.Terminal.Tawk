#ifndef APP_CLIENTS_TUI_TITLE_FLASHER_H
#define APP_CLIENTS_TUI_TITLE_FLASHER_H

#include <stdint.h>

#include "contracts/i_terminal_title.h"
#include "core/settings.h"
#include "clients/tui/tab_status.h"

/* Keeps the terminal tab/window title up to date with a status summary
 * ("tawk 🟢 🔕 · 💬 3  🖼 1  🔊 1") and flashes it after a notification until the user
 * interacts. Borrows the title writer. */
typedef struct TitleFlasher {
    ITerminalTitle *title;
    char            last[512];
    int64_t         flashing_since_ms;   /* 0 when not flashing */
} TitleFlasher;

void title_flasher_init(TitleFlasher *flasher, ITerminalTitle *title);
void title_flasher_start(TitleFlasher *flasher, int64_t now_ms);
void title_flasher_acknowledge(TitleFlasher *flasher);
void title_flasher_tick(TitleFlasher *flasher, int64_t now_ms, const TabStatus *status,
                        const Settings *settings);

#endif
