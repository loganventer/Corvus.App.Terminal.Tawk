#ifndef APP_CLIENTS_TUI_EMOJI_PICKER_PURPOSE_H
#define APP_CLIENTS_TUI_EMOJI_PICKER_PURPOSE_H

typedef enum EmojiPickerPurpose {
    EMOJI_PICKER_FOR_INPUT = 0,   /* insert at the caret */
    EMOJI_PICKER_FOR_REACTION     /* react to a message */
} EmojiPickerPurpose;

#endif
