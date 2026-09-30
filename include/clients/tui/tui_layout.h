#ifndef APP_CLIENTS_TUI_TUI_LAYOUT_H
#define APP_CLIENTS_TUI_TUI_LAYOUT_H

#include "clients/tui/ui_rect.h"

/* Screen regions, recomputed on every resize. */
typedef struct TuiLayout {
    UiRect header;
    UiRect sidebar;     /* w == 0 when collapsed or the terminal is narrow */
    UiRect divider;
    UiRect chat;
    UiRect composer;
    UiRect footer;
    UiRect body;        /* everything between header and footer */
} TuiLayout;

/* composer_text_rows: lines the input needs (1 to 5); the conversation
 * pane shrinks as the input grows. */
void tui_layout_compute(TuiLayout *layout, int rows, int cols, int sidebar_width, int sidebar_collapsed,
                        int composer_text_rows);

#endif
