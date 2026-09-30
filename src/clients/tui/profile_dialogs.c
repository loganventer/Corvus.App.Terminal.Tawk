#include "clients/tui/profile_dialogs.h"

#include <string.h>

void profile_dialogs_open(ProfileDialogs *d) {
    memset(d, 0, sizeof(*d));
    profile_view_dialog_open(&d->view);
}

void profile_dialogs_close(ProfileDialogs *d) {
    d->view.open = d->text.open = d->photo.open = 0;
    d->caret.visible = 0;
}

int profile_dialogs_is_open(const ProfileDialogs *d) { return d->view.open; }
int profile_dialogs_editing(const ProfileDialogs *d) { return d->view.open && d->text.open; }

/* The view chose a field: text fields need their current value from the
 * owner; the photo opens its menu here. */
static ProfileDialogsRequest chose_field(ProfileDialogs *d, int has_camera, int has_photo) {
    if (profile_view_dialog_choice(&d->view) == PROFILE_FIELD_PICTURE) {
        profile_photo_menu_open(&d->photo, has_camera, has_photo);
        return PROFILE_REQUEST_REDRAW;
    }
    return PROFILE_REQUEST_EDIT_TEXT;
}

static ProfileDialogsRequest from_view(ProfileDialogs *d, PopupResult r, int has_camera, int has_photo) {
    switch (r) {
        case POPUP_CHOSEN:  return chose_field(d, has_camera, has_photo);
        case POPUP_CLOSED:  profile_dialogs_close(d); return PROFILE_REQUEST_CLOSED;
        case POPUP_CHANGED: return PROFILE_REQUEST_REDRAW;
        default:            return PROFILE_REQUEST_NONE;
    }
}

static ProfileDialogsRequest from_text(PopupResult r) {
    switch (r) {
        case POPUP_CHOSEN: return PROFILE_REQUEST_SAVE_TEXT;
        case POPUP_NONE:   return PROFILE_REQUEST_NONE;
        default:           return PROFILE_REQUEST_REDRAW;   /* edited, or back to the view */
    }
}

static ProfileDialogsRequest from_photo(PopupResult r) {
    switch (r) {
        case POPUP_CHOSEN: return PROFILE_REQUEST_PHOTO;
        case POPUP_NONE:   return PROFILE_REQUEST_NONE;
        default:           return PROFILE_REQUEST_REDRAW;
    }
}

ProfileDialogsRequest profile_dialogs_key(ProfileDialogs *d, int is_key, int ch, int has_camera, int has_photo) {
    if (d->text.open) return from_text(profile_text_dialog_key(&d->text, is_key, ch));
    if (d->photo.open) return from_photo(profile_photo_menu_key(&d->photo, is_key, ch));
    return from_view(d, profile_view_dialog_key(&d->view, is_key, ch), has_camera, has_photo);
}

ProfileDialogsRequest profile_dialogs_click(ProfileDialogs *d, int y, int x, int has_camera, int has_photo) {
    if (d->text.open) return from_text(profile_text_dialog_click(&d->text, y, x));
    if (d->photo.open) return from_photo(profile_photo_menu_click(&d->photo, y, x));
    return from_view(d, profile_view_dialog_click(&d->view, y, x), has_camera, has_photo);
}

void profile_dialogs_paste(ProfileDialogs *d, const char *utf8) {
    if (d->text.open) profile_text_dialog_paste(&d->text, utf8);
}

void profile_dialogs_edit_text(ProfileDialogs *d, const char *current, int max_chars) {
    profile_text_dialog_open(&d->text, profile_view_dialog_choice(&d->view), current, max_chars);
}

void profile_dialogs_text_saved(ProfileDialogs *d) { profile_text_dialog_close(&d->text); }
void profile_dialogs_text_refused(ProfileDialogs *d, const char *why) { profile_text_dialog_error(&d->text, why); }

ProfileField profile_dialogs_field(const ProfileDialogs *d) {
    return d->text.open ? d->text.field : profile_view_dialog_choice(&d->view);
}

char *profile_dialogs_text(const ProfileDialogs *d) { return profile_text_dialog_text(&d->text); }
ProfilePhotoChoice profile_dialogs_photo_choice(const ProfileDialogs *d) { return profile_photo_menu_choice(&d->photo); }

void profile_dialogs_render(ProfileDialogs *d, UiRect area, const ProfileViewModel *model, ThumbnailCache *thumbs) {
    d->caret.visible = 0;
    if (!d->view.open) return;
    profile_view_dialog_render(&d->view, area, model, thumbs);
    if (d->text.open) profile_text_dialog_render(&d->text, area, &d->caret);
    else if (d->photo.open) profile_photo_menu_render(&d->photo, area);
}
