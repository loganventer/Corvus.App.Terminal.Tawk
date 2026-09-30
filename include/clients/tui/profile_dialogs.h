#ifndef APP_CLIENTS_TUI_PROFILE_DIALOGS_H
#define APP_CLIENTS_TUI_PROFILE_DIALOGS_H

#include "clients/tui/profile_dialogs_request.h"
#include "clients/tui/profile_photo_menu.h"
#include "clients/tui/profile_text_dialog.h"
#include "clients/tui/profile_view_dialog.h"
#include "clients/tui/text_caret.h"
#include "clients/tui/thumbnail_cache.h"

/* The dialogs for your own profile, as one popup: the profile view, with
 * the name and about editor or the photo menu over it. Routes keys and
 * clicks to the topmost one; Esc steps back to the view, then closes.
 * Knows nothing about saving: it asks its owner through requests. */
typedef struct ProfileDialogs {
    ProfileViewDialog view;
    ProfileTextDialog text;
    ProfilePhotoMenu  photo;
    TextCaret         caret;
} ProfileDialogs;

void  profile_dialogs_open(ProfileDialogs *dialogs);
void  profile_dialogs_close(ProfileDialogs *dialogs);
int   profile_dialogs_is_open(const ProfileDialogs *dialogs);
/* True while text is being edited (for the caret and for paste). */
int   profile_dialogs_editing(const ProfileDialogs *dialogs);

ProfileDialogsRequest profile_dialogs_key(ProfileDialogs *dialogs, int is_key_code, int ch, int has_camera, int has_photo);
ProfileDialogsRequest profile_dialogs_click(ProfileDialogs *dialogs, int y, int x, int has_camera, int has_photo);
void                  profile_dialogs_paste(ProfileDialogs *dialogs, const char *utf8);

/* After PROFILE_REQUEST_EDIT_TEXT: opens the editor on the current value. */
void               profile_dialogs_edit_text(ProfileDialogs *dialogs, const char *current, int max_chars);
/* After PROFILE_REQUEST_SAVE_TEXT: the change went out (back to the view), or why not. */
void               profile_dialogs_text_saved(ProfileDialogs *dialogs);
void               profile_dialogs_text_refused(ProfileDialogs *dialogs, const char *why);
ProfileField       profile_dialogs_field(const ProfileDialogs *dialogs);
/* The edited text as UTF-8; the caller frees it. */
char              *profile_dialogs_text(const ProfileDialogs *dialogs);
ProfilePhotoChoice profile_dialogs_photo_choice(const ProfileDialogs *dialogs);

void  profile_dialogs_render(ProfileDialogs *dialogs, UiRect area, const ProfileViewModel *model, ThumbnailCache *thumbs);

#endif
