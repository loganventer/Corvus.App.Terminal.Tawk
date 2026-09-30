#ifndef APP_CLIENTS_TUI_STATUS_COMPOSER_DIALOG_H
#define APP_CLIENTS_TUI_STATUS_COMPOSER_DIALOG_H

#include <stdint.h>

#include "clients/tui/status_composer_request.h"
#include "clients/tui/text_caret.h"
#include "clients/tui/text_field.h"
#include "clients/tui/ui_rect.h"
#include "core/status_kind.h"

/* Writing a status: tabs for text, photo, video and link; the words (or
 * caption); the file for a photo or video; the background colour for a
 * text or link status; and Post. Posting itself is its owner's job. */
typedef struct StatusComposerDialog {
    int        open;
    StatusKind kind;
    TextField  input;
    int        background;          /* index into the owner's palette */
    char       path[1024];          /* PHOTO, VIDEO: the chosen file */
    char       error[200];
    int        busy;                /* a post is on its way */
    TextCaret  caret;
    UiRect     last_rect;
    UiRect     tabs[STATUS_KIND_COUNT];
    UiRect     choose_button;
    UiRect     camera_button;
    UiRect     background_button;
    UiRect     post_button;
    UiRect     cancel_button;
} StatusComposerDialog;

void  status_composer_dialog_open(StatusComposerDialog *dialog, int max_chars);
StatusComposerRequest status_composer_dialog_key(StatusComposerDialog *dialog, int is_key_code, int ch);
StatusComposerRequest status_composer_dialog_click(StatusComposerDialog *dialog, int y, int x);
void  status_composer_dialog_paste(StatusComposerDialog *dialog, const char *utf8);
/* A photo or video chosen for it; switches to the matching tab when `kind` is PHOTO or VIDEO. */
void  status_composer_dialog_set_file(StatusComposerDialog *dialog, const char *path, StatusKind kind);
void  status_composer_dialog_error(StatusComposerDialog *dialog, const char *why);
/* The words or caption as UTF-8; the caller frees it. */
char *status_composer_dialog_text(const StatusComposerDialog *dialog);
/* `background_name` names the current background colour; `has_camera` shows Take photo or Record video. */
void  status_composer_dialog_render(StatusComposerDialog *dialog, UiRect area, const char *background_name,
                                    uint32_t background_argb, int has_camera);

#endif
