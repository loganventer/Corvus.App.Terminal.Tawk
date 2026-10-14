#ifndef APP_CLIENTS_TUI_SOUND_PICKER_H
#define APP_CLIENTS_TUI_SOUND_PICKER_H

#include "contracts/i_sound_catalog.h"

/* What a key, click or wheel step did to the picker. */
typedef enum SoundPickerOutcome {
    SOUND_PICKER_OPEN = 0,   /* still open, nothing to do */
    SOUND_PICKER_MOVED,      /* the highlighted sound changed: play it */
    SOUND_PICKER_CHOSEN,     /* the highlighted sound was chosen; the picker closed */
    SOUND_PICKER_CLOSED      /* closed without choosing */
} SoundPickerOutcome;

/* The state of the list of sounds in Settings. It holds no sounds and draws
 * nothing: the catalog is passed in and sound_picker_view draws it. */
typedef struct SoundPicker {
    int active;
    int index;
    int scroll;
    /* Where the view drew the list, for clicks. */
    int list_y;
    int list_h;
} SoundPicker;

/* Opens the picker with the sound at current_path highlighted (the first when
 * it is not in the catalog). */
void sound_picker_begin(SoundPicker *picker, ISoundCatalog *catalog, const char *current_path);
SoundPickerOutcome sound_picker_key(SoundPicker *picker, ISoundCatalog *catalog, int is_key_code, int ch);
SoundPickerOutcome sound_picker_wheel(SoundPicker *picker, ISoundCatalog *catalog, int delta);
/* A click on a row highlights it; a click on the highlighted row chooses it. */
SoundPickerOutcome sound_picker_click(SoundPicker *picker, ISoundCatalog *catalog, int y);
/* The highlighted sound, or NULL when the catalog is empty. */
const SoundOption *sound_picker_current(const SoundPicker *picker, ISoundCatalog *catalog);

#endif
