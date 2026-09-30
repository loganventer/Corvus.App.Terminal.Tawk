#ifndef APP_CLIENTS_TUI_SETTINGS_PANEL_H
#define APP_CLIENTS_TUI_SETTINGS_PANEL_H

#include <wchar.h>

#include "clients/tui/menu_node.h"
#include "clients/tui/settings_panel_host.h"
#include "clients/tui/text_caret.h"
#include "clients/tui/ui_rect.h"

#define SETTINGS_PANEL_DEPTH 6
#define SETTINGS_PANEL_ROWS  128

/* Android-style settings: nested menus with breadcrumbs, toggles, steppers,
 * inline text editing and a theme picker with live preview. */
typedef struct SettingsPanel {
    SettingsPanelHost host;
    const MenuNode   *stack[SETTINGS_PANEL_DEPTH];
    int               selected[SETTINGS_PANEL_DEPTH];
    int               scroll[SETTINGS_PANEL_DEPTH];
    int               depth;
    int               open;

    int               picking_theme;
    int               theme_index;
    int               theme_scroll;
    char              theme_before[48];

    int               editing;
    wchar_t           edit[512];
    int               edit_len;
    TextCaret         caret;          /* the blinking cursor while editing a text value */

    char              toast[128];
    long long         toast_until_ms;

    /* Hit-testing from the last render */
    int               row_item[SETTINGS_PANEL_ROWS];   /* screen row -> item index or -1 */
    int               crumb_x[SETTINGS_PANEL_DEPTH + 1];
    UiRect            last_rect;
} SettingsPanel;

void settings_panel_init(SettingsPanel *panel, SettingsPanelHost host);
void settings_panel_open(SettingsPanel *panel);
void settings_panel_close(SettingsPanel *panel);
void settings_panel_render(SettingsPanel *panel, UiRect rect, long long now_ms);
/* Returns 0 while open, 1 once the panel closed. */
int  settings_panel_key(SettingsPanel *panel, int is_key_code, int ch, long long now_ms);
int  settings_panel_click(SettingsPanel *panel, int y, int x, long long now_ms);
void settings_panel_wheel(SettingsPanel *panel, int delta);
/* Accepts text dropped or pasted while editing a field. */
void settings_panel_paste(SettingsPanel *panel, const char *utf8);

#endif
