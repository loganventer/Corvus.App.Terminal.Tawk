#ifndef APP_CLIENTS_TUI_PROFILE_PHOTO_CHOICE_H
#define APP_CLIENTS_TUI_PROFILE_PHOTO_CHOICE_H

/* What the profile photo menu offers. */
typedef enum ProfilePhotoChoice {
    PROFILE_PHOTO_CHOOSE_FILE = 0,
    PROFILE_PHOTO_TAKE_PHOTO,
    PROFILE_PHOTO_PASTE,
    PROFILE_PHOTO_VIEW,
    PROFILE_PHOTO_REMOVE,
    PROFILE_PHOTO_COUNT
} ProfilePhotoChoice;

const char *profile_photo_choice_label(ProfilePhotoChoice choice);

#endif
