#ifndef APP_CLIENTS_TUI_ATTACH_CHOICE_H
#define APP_CLIENTS_TUI_ATTACH_CHOICE_H

/* What the + button offers. */
typedef enum AttachChoice {
    ATTACH_CHOICE_PHOTO = 0,     /* take a photo with the camera */
    ATTACH_CHOICE_FILE,          /* choose a file */
    ATTACH_CHOICE_COUNT
} AttachChoice;

const char *attach_choice_label(AttachChoice choice);

#endif
