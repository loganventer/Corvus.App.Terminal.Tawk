#ifndef APP_CLIENTS_TUI_PROFILE_PHOTO_MENU_H
#define APP_CLIENTS_TUI_PROFILE_PHOTO_MENU_H

#include "clients/tui/popup_result.h"
#include "clients/tui/profile_photo_choice.h"
#include "clients/tui/ui_rect.h"

/* The choices for your profile photo: a new one from a file, the camera or
 * the clipboard, seeing it full size, or removing it. */
typedef struct ProfilePhotoMenu {
    int    open;
    int    has_camera;
    int    has_photo;       /* without one, View and Remove are dimmed */
    int    selected;
    UiRect last_rect;
} ProfilePhotoMenu;

void               profile_photo_menu_open(ProfilePhotoMenu *menu, int has_camera, int has_photo);
PopupResult        profile_photo_menu_key(ProfilePhotoMenu *menu, int is_key_code, int ch);
PopupResult        profile_photo_menu_click(ProfilePhotoMenu *menu, int y, int x);
ProfilePhotoChoice profile_photo_menu_choice(const ProfilePhotoMenu *menu);
/* Centred in `area`. */
void               profile_photo_menu_render(ProfilePhotoMenu *menu, UiRect area);

#endif
