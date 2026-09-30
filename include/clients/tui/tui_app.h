#ifndef APP_CLIENTS_TUI_TUI_APP_H
#define APP_CLIENTS_TUI_TUI_APP_H

#include "clients/tui/tui_app_deps.h"

/* The terminal client: owns the widgets and the event loop. */
typedef struct TuiApp TuiApp;

TuiApp *tui_app_create(const TuiAppDeps *deps);
/* Runs until the user quits. Returns the process exit code. */
int     tui_app_run(TuiApp *app);
void    tui_app_destroy(TuiApp *app);

#endif
