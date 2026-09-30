#ifndef APP_CLIENTS_TUI_CHAT_PICKER_H
#define APP_CLIENTS_TUI_CHAT_PICKER_H

#include "clients/tui/popup_result.h"
#include "clients/tui/text_caret.h"
#include "clients/tui/text_field.h"
#include "clients/tui/ui_rect.h"
#include "core/chat.h"

#define CHAT_PICKER_MAX_CHOSEN 5     /* as many as the phone lets you forward to at once */
#define CHAT_PICKER_ROWS       64

/* Choosing up to five chats, for forwarding: a search box, the chats that
 * match it, and a tick on each chosen one. Space ticks or unticks the
 * highlighted chat, typing filters, Enter sends (the highlighted chat when
 * none is ticked) and Esc closes. Knows nothing about what is sent. */
typedef struct ChatPicker {
    int       open;
    char      title[96];
    TextField query;
    char      chosen[CHAT_PICKER_MAX_CHOSEN][128];
    int       chosen_count;
    int       full;                                     /* tried to tick a sixth */
    int       selected;                                 /* row highlighted */
    int       scroll;
    char      row_jid[CHAT_PICKER_ROWS][128];           /* the chats shown, from the last render */
    int       row_count;
    UiRect    last_rect;
    UiRect    list_rect;
    UiRect    send_button;
    TextCaret caret;
} ChatPicker;

void        chat_picker_open(ChatPicker *picker, const char *title);
void        chat_picker_close(ChatPicker *picker);
/* POPUP_CHOSEN to send to the chosen chats, POPUP_CLOSED when dismissed. */
PopupResult chat_picker_key(ChatPicker *picker, int is_key_code, int ch);
PopupResult chat_picker_click(ChatPicker *picker, int y, int x);
void        chat_picker_wheel(ChatPicker *picker, int delta);
void        chat_picker_paste(ChatPicker *picker, const char *utf8);
int         chat_picker_is_chosen(const ChatPicker *picker, const char *jid);
/* The chats chosen, as pointers into the picker; valid until it changes. */
int         chat_picker_chosen(const ChatPicker *picker, const char *out[CHAT_PICKER_MAX_CHOSEN]);
/* Lists the chats matching the search (Locked chats left out). */
void        chat_picker_render(ChatPicker *picker, UiRect area, const Chat *chats, int count);

#endif
