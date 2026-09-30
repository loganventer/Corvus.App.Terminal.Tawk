#ifndef APP_CLIENTS_TUI_EMOJI_PICKER_H
#define APP_CLIENTS_TUI_EMOJI_PICKER_H

#include "clients/tui/emoji_picker_purpose.h"
#include "clients/tui/popup_result.h"
#include "clients/tui/text_caret.h"
#include "clients/tui/ui_rect.h"
#include "contracts/i_emoji_catalog.h"

#define EMOJI_PICKER_RECENT     24
#define EMOJI_PICKER_TABS       (EMOJI_GROUP_COUNT + 1)   /* Recent + groups */
#define EMOJI_PICKER_MAX_SHOWN  8192

/* The full emoji set in one scrolling grid: recent emoji first, then every
 * group, each starting on a new row. Typing filters it by name; the group
 * tabs jump to a group and follow the selection. */
typedef struct EmojiPicker {
    int                open;
    EmojiPickerPurpose purpose;
    char               message_id[64];      /* reactions: the target message */
    char               query[64];
    int                tab;                 /* section of the selection: 0 = Recent, 1.. = EmojiGroup + 1 */
    int                shown[EMOJI_PICKER_MAX_SHOWN];   /* catalog indexes; -1 pads a row */
    int                section_start[EMOJI_PICKER_TABS];  /* first shown index per section, -1 if empty */
    int                shown_count;
    int                selected;            /* index into shown */
    int                scroll_row;
    int                cols;                /* grid columns of the last render */
    char               recent[EMOJI_PICKER_RECENT][48];
    int                recent_count;
    int                tab_x[EMOJI_PICKER_TABS + 1];
    UiRect             grid;
    UiRect             last_rect;
    TextCaret          caret;               /* the blinking cursor in the search field */
} EmojiPicker;

/* recent_csv: previously used emoji separated by spaces (from settings). */
void        emoji_picker_open(EmojiPicker *picker, IEmojiCatalog *catalog, EmojiPickerPurpose purpose,
                              const char *message_id, const char *recent_csv, const char *query);
PopupResult emoji_picker_key(EmojiPicker *picker, IEmojiCatalog *catalog, int is_key_code, int ch);
PopupResult emoji_picker_click(EmojiPicker *picker, IEmojiCatalog *catalog, int y, int x);
void        emoji_picker_wheel(EmojiPicker *picker, int delta);
/* The chosen emoji, valid after POPUP_CHOSEN. */
const char *emoji_picker_choice(const EmojiPicker *picker, IEmojiCatalog *catalog);
void        emoji_picker_render(EmojiPicker *picker, UiRect area, IEmojiCatalog *catalog);

#endif
