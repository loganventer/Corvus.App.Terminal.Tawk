#ifndef APP_CLIENTS_TUI_SETTINGS_PANEL_HOST_H
#define APP_CLIENTS_TUI_SETTINGS_PANEL_HOST_H

#include <stddef.h>

#include "clients/tui/menu_action.h"
#include "clients/tui/menu_info.h"
#include "contracts/i_theme_repository.h"
#include "core/settings.h"

/* What the settings panel needs from the application; injected so the panel
 * stays a pure view. */
typedef struct SettingsPanelHost {
    void *ctx;
    const Settings   *(*settings)(void *ctx);
    int               (*apply)(void *ctx, const Settings *updated);
    IThemeRepository *(*themes)(void *ctx);
    void              (*preview_theme)(void *ctx, const Theme *theme);
    void              (*run_action)(void *ctx, MenuAction action);
    void              (*info)(void *ctx, MenuInfo info, char *out, size_t size);
    int               (*agent_connected)(void *ctx);   /* a program acting for a model is on the control socket now */
} SettingsPanelHost;

#endif
