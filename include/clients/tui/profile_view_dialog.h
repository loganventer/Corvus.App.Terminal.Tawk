#ifndef APP_CLIENTS_TUI_PROFILE_VIEW_DIALOG_H
#define APP_CLIENTS_TUI_PROFILE_VIEW_DIALOG_H

#include "clients/tui/popup_result.h"
#include "clients/tui/profile_view_model.h"
#include "clients/tui/thumbnail_cache.h"
#include "clients/tui/ui_rect.h"
#include "core/profile_field.h"

/* Your profile, like WhatsApp's Profile screen: the photo, then your name,
 * about text and phone number. Name, About and Photo can each be chosen to
 * change them. */
typedef struct ProfileViewDialog {
    int    open;
    int    selected;                        /* a ProfileField */
    UiRect last_rect;
    UiRect portrait_rect;
    UiRect rows[PROFILE_FIELD_COUNT];
} ProfileViewDialog;

void         profile_view_dialog_open(ProfileViewDialog *dialog);
/* POPUP_CHOSEN when a field was chosen to change, POPUP_CLOSED when dismissed. */
PopupResult  profile_view_dialog_key(ProfileViewDialog *dialog, int is_key_code, int ch);
PopupResult  profile_view_dialog_click(ProfileViewDialog *dialog, int y, int x);
ProfileField profile_view_dialog_choice(const ProfileViewDialog *dialog);
void         profile_view_dialog_render(ProfileViewDialog *dialog, UiRect area, const ProfileViewModel *model,
                                        ThumbnailCache *thumbs);

#endif
