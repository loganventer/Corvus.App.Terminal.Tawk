#ifndef APP_CLIENTS_TUI_PROFILE_DIALOGS_REQUEST_H
#define APP_CLIENTS_TUI_PROFILE_DIALOGS_REQUEST_H

/* What the profile dialogs need their owner to do after a key or click. */
typedef enum ProfileDialogsRequest {
    PROFILE_REQUEST_NONE = 0,
    PROFILE_REQUEST_REDRAW,        /* something changed on screen */
    PROFILE_REQUEST_CLOSED,        /* all of them were dismissed */
    PROFILE_REQUEST_EDIT_TEXT,     /* start editing profile_dialogs_field (call _edit_text) */
    PROFILE_REQUEST_SAVE_TEXT,     /* save profile_dialogs_text as profile_dialogs_field */
    PROFILE_REQUEST_PHOTO          /* act on profile_dialogs_photo_choice */
} ProfileDialogsRequest;

#endif
