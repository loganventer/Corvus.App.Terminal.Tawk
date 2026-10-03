#ifndef APP_CLIENTS_TUI_CHAT_TOGGLE_DIALOG_H
#define APP_CLIENTS_TUI_CHAT_TOGGLE_DIALOG_H

#include "clients/tui/popup_result.h"
#include "clients/tui/text_caret.h"
#include "clients/tui/text_field.h"
#include "clients/tui/ui_rect.h"
#include "core/chat.h"

#define CHAT_TOGGLE_CAPACITY 24      /* chats that can be switched on one by one */
#define CHAT_TOGGLE_ROWS     64

/* A list of chats, each with a switch, kept as a setting: an "All chats"
 * switch on top, then a search box's matches. Space or a click flips the
 * highlighted switch, Ctrl+A flips All chats, typing filters, Enter saves
 * and Esc leaves things as they were. Knows nothing about what the chats
 * are switched on for. */
typedef struct ChatToggleDialog {
    int       open;
    char      title[96];
    char      all_label[64];                            /* what "All chats" means here */
    TextField query;
    int       all;                                      /* every chat, now and later */
    char      on[CHAT_TOGGLE_CAPACITY][128];            /* the chats switched on one by one */
    int       on_count;
    int       full;                                     /* tried to switch on one too many */
    int       selected;                                 /* row highlighted; 0 is All chats */
    int       scroll;
    char      row_jid[CHAT_TOGGLE_ROWS][128];           /* the chats shown, from the last render */
    int       row_count;
    UiRect    last_rect;
    UiRect    all_rect;
    UiRect    list_rect;
    UiRect    save_button;
    TextCaret caret;
} ChatToggleDialog;

void        chat_toggle_dialog_open(ChatToggleDialog *dialog, const char *title, const char *all_label);
void        chat_toggle_dialog_close(ChatToggleDialog *dialog);
/* What is switched on when the dialog opens. */
void        chat_toggle_dialog_set_all(ChatToggleDialog *dialog, int all);
int         chat_toggle_dialog_set_on(ChatToggleDialog *dialog, const char *jid);
/* POPUP_CHOSEN to save what is switched on, POPUP_CLOSED when dismissed. */
PopupResult chat_toggle_dialog_key(ChatToggleDialog *dialog, int is_key_code, int ch);
PopupResult chat_toggle_dialog_click(ChatToggleDialog *dialog, int y, int x);
void        chat_toggle_dialog_wheel(ChatToggleDialog *dialog, int delta);
void        chat_toggle_dialog_paste(ChatToggleDialog *dialog, const char *utf8);
int         chat_toggle_dialog_all(const ChatToggleDialog *dialog);
int         chat_toggle_dialog_is_on(const ChatToggleDialog *dialog, const char *jid);
/* The chats switched on one by one, as pointers into the dialog; valid until it changes. */
int         chat_toggle_dialog_chats(const ChatToggleDialog *dialog, const char **out, int max);
/* Lists the chats matching the search (Locked chats left out). */
void        chat_toggle_dialog_render(ChatToggleDialog *dialog, UiRect area, const Chat *chats, int count);

#endif
