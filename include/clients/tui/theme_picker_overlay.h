#ifndef APP_CLIENTS_TUI_THEME_PICKER_OVERLAY_H
#define APP_CLIENTS_TUI_THEME_PICKER_OVERLAY_H

#include "clients/tui/popup_result.h"
#include "clients/tui/ui_rect.h"
#include "contracts/i_theme_repository.h"

#define THEME_PICKER_ROWS 128

/* Chooses a theme for one chat, previewing each on the conversation.
 * The first entry means "use the app theme". */
typedef struct ThemePickerOverlay {
    int    open;
    char   jid[128];
    char   title[128];
    int    position;           /* 0 = app theme, n = theme n - 1 */
    int    scroll;
    int    original;           /* position when opened, for Esc */
    int    row_item[THEME_PICKER_ROWS];
    UiRect last_rect;
} ThemePickerOverlay;

void        theme_picker_overlay_open(ThemePickerOverlay *picker, IThemeRepository *themes,
                                      const char *jid, const char *chat_name, const char *current_id);
PopupResult theme_picker_overlay_key(ThemePickerOverlay *picker, IThemeRepository *themes, int is_key_code, int ch);
PopupResult theme_picker_overlay_click(ThemePickerOverlay *picker, int y, int x);
void        theme_picker_overlay_wheel(ThemePickerOverlay *picker, IThemeRepository *themes, int delta);
/* Theme at the selected position, or NULL for the app theme. */
const Theme *theme_picker_overlay_selected(const ThemePickerOverlay *picker, IThemeRepository *themes);
void        theme_picker_overlay_render(ThemePickerOverlay *picker, UiRect area, IThemeRepository *themes);

#endif
