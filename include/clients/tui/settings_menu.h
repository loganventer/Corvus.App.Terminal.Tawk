#ifndef APP_CLIENTS_TUI_SETTINGS_MENU_H
#define APP_CLIENTS_TUI_SETTINGS_MENU_H

#include "clients/tui/menu_node.h"

/* The settings tree, modelled on the Android WhatsApp settings screens. */
const MenuNode *settings_menu_root(void);

/* What a menu shows now. Settings that only mean something to a connected agent
 * give way to one line saying none is connected. */
const MenuNode *settings_menu_shown(const MenuNode *menu, int agent_connected);

#endif
