#ifndef APP_CLIENTS_TUI_STATUS_VIEWER_DIALOG_H
#define APP_CLIENTS_TUI_STATUS_VIEWER_DIALOG_H

#include "clients/tui/media_sources.h"
#include "clients/tui/popup_result.h"
#include "clients/tui/status_viewer_intent.h"
#include "clients/tui/text_caret.h"
#include "clients/tui/text_field.h"
#include "clients/tui/thumbnail_cache.h"
#include "clients/tui/ui_rect.h"
#include "core/status_update.h"

/* One person's statuses, one at a time: a bar per status along the top,
 * who and when, then the status itself (words on their colour, or the
 * photo or video with its caption). Left and Right step through them. */
typedef struct StatusViewerDialog {
    int    open;
    char   author_jid[128];
    char   title[128];
    int    index;
    int    count;           /* statuses in the last render */
    int    moved;           /* index changed since the owner last looked */
    UiRect last_rect;
    UiRect media_rect;
    UiRect prev_zone;
    UiRect next_zone;
    UiRect viewers_button;   /* "Seen by 5 · ❤ 2" under your own statuses */
    int    mine;             /* the status in view is yours (set while drawing) */
    StatusViewerIntent intent;       /* asked for by the last key or click; taken by the owner */
    int    intent_emoji;     /* STATUS_VIEWER_INTENT_REACT: which quick emoji */
    int    replying;         /* typing a reply to someone else's status */
    TextField reply;
    TextCaret caret;
    UiRect reactions[8];     /* the quick emoji under someone else's status */
    UiRect like_button;
    UiRect reply_button;
} StatusViewerDialog;

/* `start` is the first status to show (the first unseen one, say). */
void        status_viewer_dialog_open(StatusViewerDialog *dialog, const char *author_jid, const char *title, int start, int count);
/* POPUP_CHANGED when another status came into view, POPUP_CHOSEN to open
 * the photo or video, POPUP_CLOSED back to the list (Esc, or past the last). */
PopupResult status_viewer_dialog_key(StatusViewerDialog *dialog, int is_key_code, int ch);
PopupResult status_viewer_dialog_click(StatusViewerDialog *dialog, int y, int x);
/* The quick emoji offered under other people's statuses, as on the phone. */
const char *status_viewer_dialog_quick_emoji(int index);
/* The reply typed, as UTF-8; the caller frees it. */
char       *status_viewer_dialog_reply_text(const StatusViewerDialog *dialog);
void        status_viewer_dialog_paste(StatusViewerDialog *dialog, const char *utf8);
/* `viewers_label` ("Seen by 5 · ❤ 2") is drawn as a button under your own statuses; NULL for other people's,
 * which get the quick emoji, a like and a reply instead. */
void        status_viewer_dialog_render(StatusViewerDialog *dialog, UiRect area, const StatusUpdate *items, int count,
                                        ThumbnailCache *thumbs, const MediaSources *sources, int use_24h,
                                        const char *viewers_label);

#endif
