#ifndef APP_CLIENTS_TUI_MENU_NODE_H
#define APP_CLIENTS_TUI_MENU_NODE_H

#include "clients/tui/menu_action.h"
#include "clients/tui/menu_info.h"
#include "clients/tui/menu_node_kind.h"
#include "core/setting_category.h"

/* One entry in the settings menu tree. */
typedef struct MenuNode {
    MenuNodeKind           kind;
    const char            *icon;
    const char            *title;
    const char            *subtitle;     /* submenus: summary under the title */
    SettingCategory        category;     /* FIELD */
    const char            *key;          /* FIELD */
    MenuAction             action;       /* ACTION */
    MenuInfo               info;         /* INFO */
    const struct MenuNode *children;     /* SUBMENU */
    int                    child_count;
} MenuNode;

#endif
