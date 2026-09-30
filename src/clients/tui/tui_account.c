/* Your own profile and statuses: what the profile dialogs and the status
 * composer ask for, carried out through the account and status managers. */
#include "tui_app_state.h"
#include "core/contact_profile.h"
#include "core/status_post.h"
#include "utilities/str_util.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static const Settings *settings(TuiApp *app) { return settings_manager_current(app->deps.settings); }

const char *tui_app_user_name(TuiApp *app) {
    const char *changed = app->deps.accounts ? account_manager_user_name(app->deps.accounts) : "";
    return changed && *changed ? changed : messaging_manager_user_name(app->deps.messaging);
}

static const char *own_jid(TuiApp *app) {
    const char *jid = app->deps.accounts ? account_manager_user_jid(app->deps.accounts) : "";
    return jid && *jid ? jid : messaging_manager_user_jid(app->deps.messaging);
}

/* ---- profile ------------------------------------------------------------ */

void tui_app_open_profile(TuiApp *app) {
    if (tui_app_show_login(app) || !own_jid(app)[0]) { tui_app_toast(app, "Link your phone first", 1); return; }
    ContactProfile fresh;                                   /* asks for your about text and photo again */
    if (profile_manager_details(app->deps.profiles, own_jid(app), 1, &fresh) == 0) contact_profile_dispose(&fresh);
    profile_manager_picture(app->deps.profiles, own_jid(app));
    profile_dialogs_open(&app->profile);
    app->dirty = 1;
}

void tui_app_profile_model(TuiApp *app, ProfileViewModel *m, char *about, size_t about_size) {
    const char *jid = own_jid(app);
    about[0] = '\0';
    ContactProfile p;
    if (profile_manager_details(app->deps.profiles, jid, 0, &p) == 0) {
        str_copy(about, about_size, p.about);
        contact_profile_dispose(&p);
    }
    memset(m, 0, sizeof(*m));
    m->jid = jid;
    m->name = tui_app_user_name(app);
    m->about = about;
    m->picture = profile_manager_picture(app->deps.profiles, jid);
    for (int f = 0; f < PROFILE_FIELD_COUNT; f++) m->busy[f] = account_manager_busy(app->deps.accounts, (ProfileField)f);
}

static void edit_text(TuiApp *app) {
    ProfileField field = profile_dialogs_field(&app->profile);
    char about[700];
    ProfileViewModel m;
    tui_app_profile_model(app, &m, about, sizeof(about));
    profile_dialogs_edit_text(&app->profile, field == PROFILE_FIELD_NAME ? m.name : m.about,
                              account_manager_max_chars(app->deps.accounts, field));
}

static void save_text(TuiApp *app) {
    ProfileField field = profile_dialogs_field(&app->profile);
    char *text = profile_dialogs_text(&app->profile);
    if (!text) return;
    char *trimmed = str_trim(text);
    int rc = field == PROFILE_FIELD_NAME ? account_manager_set_name(app->deps.accounts, trimmed)
                                         : account_manager_set_about(app->deps.accounts, trimmed);
    free(text);
    if (rc != 0) { profile_dialogs_text_refused(&app->profile, account_manager_error(app->deps.accounts)); return; }
    profile_dialogs_text_saved(&app->profile);
    tui_app_toast(app, field == PROFILE_FIELD_NAME ? "Saving your name\xE2\x80\xA6" : "Saving your about\xE2\x80\xA6", 0);
}

static void set_picture(TuiApp *app, const char *path) {
    if (account_manager_set_picture(app->deps.accounts, path) != 0) {
        tui_app_toast(app, account_manager_error(app->deps.accounts), 1);
        return;
    }
    tui_app_toast(app, "Saving your photo\xE2\x80\xA6", 0);
}

static void paste_picture(TuiApp *app) {
    char path[1024];
    switch (app->deps.clipboard_image->save(app->deps.clipboard_image, settings(app)->media_dir, path, sizeof(path))) {
        case CLIPBOARD_IMAGE_SAVED: set_picture(app, path); break;
        case CLIPBOARD_IMAGE_EMPTY: tui_app_toast(app, "There is no picture on the clipboard", 1); break;
        default:                    tui_app_toast(app, "Pasting pictures needs wl-clipboard (Wayland) or xclip (X11)", 1); break;
    }
}

static void view_picture(TuiApp *app) {
    const char *jid = own_jid(app);
    const char *full = profile_manager_full_picture(app->deps.profiles, jid);
    const char *preview = profile_manager_picture(app->deps.profiles, jid);
    if (!full && !preview) { tui_app_toast(app, "You have no profile photo", 1); return; }
    image_viewer_open_portrait(&app->viewer, jid, tui_app_user_name(app), full ? full : preview);
}

static void photo_choice(TuiApp *app) {
    switch (profile_dialogs_photo_choice(&app->profile)) {
        case PROFILE_PHOTO_CHOOSE_FILE: tui_app_open_file_picker(app, FILE_PICKER_FOR_AVATAR); break;
        case PROFILE_PHOTO_TAKE_PHOTO:  tui_app_open_camera_for(app, CAMERA_FOR_AVATAR); break;
        case PROFILE_PHOTO_PASTE:       paste_picture(app); break;
        case PROFILE_PHOTO_VIEW:        view_picture(app); break;
        case PROFILE_PHOTO_REMOVE:
            confirm_dialog_open(&app->confirm, CONFIRM_REMOVE_PHOTO, own_jid(app), "Remove photo",
                                "Remove your profile photo?", "People will see your initials instead.", "Remove", 0);
            break;
        default:
            break;
    }
}

void tui_app_profile_request(TuiApp *app, ProfileDialogsRequest request) {
    switch (request) {
        case PROFILE_REQUEST_EDIT_TEXT: edit_text(app); break;
        case PROFILE_REQUEST_SAVE_TEXT: save_text(app); break;
        case PROFILE_REQUEST_PHOTO:     photo_choice(app); break;
        case PROFILE_REQUEST_NONE:      return;
        default:                        break;
    }
    app->dirty = 1;
}

void tui_app_remove_profile_photo(TuiApp *app) {
    if (account_manager_remove_picture(app->deps.accounts) != 0) tui_app_toast(app, account_manager_error(app->deps.accounts), 1);
    else tui_app_toast(app, "Removing your photo\xE2\x80\xA6", 0);
}

/* ---- statuses ----------------------------------------------------------- */

void tui_app_open_status(TuiApp *app) {
    if (tui_app_show_login(app)) return;
    if (!status_manager_supported(app->deps.statuses)) {
        if (!app->deps.whatsmeow_available) {
            tui_app_toast(app, "Posting statuses needs the whatsmeow backend, which this build of tawk does not include", 1);
            return;
        }
        confirm_dialog_open(&app->confirm, CONFIRM_USE_WHATSMEOW, "", "Posting statuses",
                            "Posting needs the whatsmeow backend. Switch now?",
                            "tawk will restart, and you will link this computer again with a QR code or pairing code. "
                            "Your chats stay.", "Switch to whatsmeow", 0);
        app->dirty = 1;
        return;
    }
    if (!app->status_composer.open) {
        status_composer_dialog_open(&app->status_composer, STATUS_POST_MAX_CHARS);
        /* Each new status starts on a different colour, as on the phone. */
        int colours = status_manager_background_count(app->deps.statuses);
        if (colours > 0) app->status_composer.background = (int)(random() % colours);
    }
    app->dirty = 1;
}

static void post_status(TuiApp *app) {
    StatusComposerDialog *d = &app->status_composer;
    StatusPost post;
    memset(&post, 0, sizeof(post));
    post.kind = d->kind;
    char *text = status_composer_dialog_text(d);
    if (text) { str_copy(post.text, sizeof(post.text), str_trim(text)); free(text); }
    if (status_kind_has_media(d->kind)) str_copy(post.path, sizeof(post.path), d->path);
    post.background_argb = status_manager_background(app->deps.statuses, d->background);
    if (status_manager_post(app->deps.statuses, &post) != 0) {
        status_composer_dialog_error(d, status_manager_error(app->deps.statuses));
        return;
    }
    d->busy = 1;
}

void tui_app_status_request(TuiApp *app, StatusComposerRequest request) {
    switch (request) {
        case STATUS_REQUEST_POST:        post_status(app); break;
        case STATUS_REQUEST_CHOOSE_FILE: tui_app_open_file_picker(app, FILE_PICKER_FOR_STATUS); break;
        case STATUS_REQUEST_CAMERA:
            tui_app_open_camera_for(app, CAMERA_FOR_STATUS);
            if (app->status_composer.kind == STATUS_KIND_VIDEO && app->camera_view.open) {
                tui_app_toast(app, "Press V to start recording, and V again to stop", 0);
            }
            break;
        case STATUS_REQUEST_NONE:        return;
        default:                         break;
    }
    app->dirty = 1;
}

/* Restarts tawk on whatsmeow: saved first, so the restart picks it up. */
void tui_app_switch_to_whatsmeow(TuiApp *app) {
    Settings s = *settings(app);
    str_copy(s.backend, sizeof(s.backend), "whatsmeow");
    if (settings_manager_apply(app->deps.settings, &s) != 0) { tui_app_toast(app, "The setting could not be saved", 1); return; }
    if (app->deps.restart_requested) *app->deps.restart_requested = 1;
    app->running = 0;
}

/* A file for the status: its type picks the Photo or Video tab. */
static void status_file(TuiApp *app, const char *path) {
    StatusKind kind = status_manager_kind_for_file(app->deps.statuses, path);
    if (!status_kind_has_media(kind)) {
        status_composer_dialog_error(&app->status_composer, "Statuses take photos and videos only");
        return;
    }
    status_composer_dialog_set_file(&app->status_composer, path, kind);
}

void tui_app_account_file(TuiApp *app, FilePickerPurpose purpose, const char *path) {
    if (purpose == FILE_PICKER_FOR_AVATAR) set_picture(app, path);
    else if (purpose == FILE_PICKER_FOR_STATUS) status_file(app, path);
    app->dirty = 1;
}

/* A picture on the clipboard becomes the photo of the status being written. */
void tui_app_status_paste_picture(TuiApp *app) {
    char path[1024];
    switch (app->deps.clipboard_image->save(app->deps.clipboard_image, settings(app)->media_dir, path, sizeof(path))) {
        case CLIPBOARD_IMAGE_SAVED: status_file(app, path); break;
        case CLIPBOARD_IMAGE_EMPTY: tui_app_toast(app, "There is no picture on the clipboard", 1); break;
        default:                    tui_app_toast(app, "Pasting pictures needs wl-clipboard (Wayland) or xclip (X11)", 1); break;
    }
    app->dirty = 1;
}

/* ---- results ------------------------------------------------------------ */

void tui_app_account_tick(TuiApp *app) {
    account_manager_tick(app->deps.accounts);
    status_manager_tick(app->deps.statuses);

    ProfileEditResult edit;
    while (account_manager_take_result(app->deps.accounts, &edit) == 0) {
        char msg[320];
        const char *what = edit.field == PROFILE_FIELD_NAME ? "name" : edit.field == PROFILE_FIELD_ABOUT ? "about" : "photo";
        if (edit.ok) snprintf(msg, sizeof(msg), "Your %s is updated", what);
        else snprintf(msg, sizeof(msg), "Could not update your %s: %s", what, edit.detail[0] ? edit.detail : "WhatsApp refused it");
        tui_app_toast(app, msg, !edit.ok);
        app->dirty = 1;
    }

    StatusPostResult posted;
    while (status_manager_take_result(app->deps.statuses, &posted) == 0) {
        StatusComposerDialog *d = &app->status_composer;
        d->busy = 0;
        if (posted.ok) {
            d->open = 0;
            tui_app_toast(app, "Status posted", 0);
        } else {
            char msg[320];
            snprintf(msg, sizeof(msg), "Could not post the status: %s", posted.detail[0] ? posted.detail : "WhatsApp refused it");
            if (d->open) status_composer_dialog_error(d, msg);
            tui_app_toast(app, msg, 1);
        }
        app->dirty = 1;
    }
}
