#ifndef APP_CLIENTS_TUI_FILE_PICKER_PURPOSE_H
#define APP_CLIENTS_TUI_FILE_PICKER_PURPOSE_H

/* What the chosen file is for. */
typedef enum FilePickerPurpose {
    FILE_PICKER_FOR_ATTACHMENT = 0,
    FILE_PICKER_FOR_TONE,             /* a chat's notification sound */
    FILE_PICKER_FOR_AVATAR,           /* your profile photo */
    FILE_PICKER_FOR_STATUS            /* the photo or video of a status */
} FilePickerPurpose;

#endif
