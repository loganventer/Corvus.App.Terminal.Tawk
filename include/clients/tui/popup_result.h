#ifndef APP_CLIENTS_TUI_POPUP_RESULT_H
#define APP_CLIENTS_TUI_POPUP_RESULT_H

/* What a key or click did in a popup (search, palette, menus, pickers). */
typedef enum PopupResult {
    POPUP_NONE = 0,      /* still open, nothing to act on */
    POPUP_CHANGED,       /* selection or query changed (preview, re-query) */
    POPUP_CHOSEN,        /* the user picked the selected item */
    POPUP_CLOSED         /* dismissed without a choice */
} PopupResult;

#endif
