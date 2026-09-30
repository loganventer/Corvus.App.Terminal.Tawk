#ifndef APP_CLIENTS_TUI_PROFILE_TEXT_DIALOG_H
#define APP_CLIENTS_TUI_PROFILE_TEXT_DIALOG_H

#include "clients/tui/popup_result.h"
#include "clients/tui/text_caret.h"
#include "clients/tui/text_field.h"
#include "clients/tui/ui_rect.h"
#include "core/profile_field.h"

/* Changes your name or about text: the text in an editable box, how many
 * characters are left, and Save and Cancel. What makes the text acceptable
 * is not decided here; the owner shows the reason with _error. */
typedef struct ProfileTextDialog {
    int          open;
    ProfileField field;
    TextField    input;
    char         error[200];
    UiRect       last_rect;
    UiRect       save_button;
    UiRect       cancel_button;
} ProfileTextDialog;

void        profile_text_dialog_open(ProfileTextDialog *dialog, ProfileField field, const char *current, int max_chars);
/* POPUP_CHOSEN to save (the dialog stays open until closed), POPUP_CLOSED when cancelled. */
PopupResult profile_text_dialog_key(ProfileTextDialog *dialog, int is_key_code, int ch);
PopupResult profile_text_dialog_click(ProfileTextDialog *dialog, int y, int x);
void        profile_text_dialog_paste(ProfileTextDialog *dialog, const char *utf8);
/* The text to save, as UTF-8; the caller frees it. */
char       *profile_text_dialog_text(const ProfileTextDialog *dialog);
void        profile_text_dialog_error(ProfileTextDialog *dialog, const char *why);
void        profile_text_dialog_close(ProfileTextDialog *dialog);
void        profile_text_dialog_render(ProfileTextDialog *dialog, UiRect area, TextCaret *caret);

#endif
