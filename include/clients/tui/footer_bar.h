#ifndef APP_CLIENTS_TUI_FOOTER_BAR_H
#define APP_CLIENTS_TUI_FOOTER_BAR_H

#include "clients/tui/ui_rect.h"

/* Bottom line: context hints, or a transient message when one is active. */
void footer_bar_render(UiRect rect, const char *hints, const char *toast, int toast_is_error);

#endif
