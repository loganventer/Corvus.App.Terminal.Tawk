#ifndef APP_CLIENTS_TUI_SOUND_PICKER_VIEW_H
#define APP_CLIENTS_TUI_SOUND_PICKER_VIEW_H

#include "clients/tui/sound_picker.h"
#include "clients/tui/ui_rect.h"

/* Draws the sounds in `list`, marking the one stored at current_path, and
 * keeps the highlighted row in view. */
void sound_picker_view_render(SoundPicker *picker, ISoundCatalog *catalog, UiRect list, const char *current_path);

#endif
