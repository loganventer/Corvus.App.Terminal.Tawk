#ifndef APP_CLIENTS_TUI_MENU_NODE_KIND_H
#define APP_CLIENTS_TUI_MENU_NODE_KIND_H

typedef enum MenuNodeKind {
    MENU_NODE_SUBMENU = 0,  /* opens child nodes */
    MENU_NODE_FIELD,        /* edits one Settings field */
    MENU_NODE_THEMES,       /* theme picker with live preview */
    MENU_NODE_SOUNDS,       /* notification sound picker, each plays as you move */
    MENU_NODE_ACTION,       /* runs a command (log out, test sound, ...) */
    MENU_NODE_INFO          /* read-only line */
} MenuNodeKind;

#endif
