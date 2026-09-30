#ifndef APP_CLIENTS_TUI_TEXT_FIELD_H
#define APP_CLIENTS_TUI_TEXT_FIELD_H

#include <wchar.h>

#include "clients/tui/text_caret.h"
#include "clients/tui/ui_rect.h"

#define TEXT_FIELD_CAPACITY 1024

/* A box of editable text for dialogs: typing, Backspace and Delete, the
 * arrow keys, Home and End, and paste. Text longer than a row wraps onto
 * the rows the field is given and scrolls to keep the cursor in view.
 * Enter and Esc are left to the dialog that owns the field. */
typedef struct TextField {
    wchar_t text[TEXT_FIELD_CAPACITY + 1];
    int     length;
    int     cursor;
    int     max_chars;    /* at most this many characters */
    int     multiline;    /* Shift+Enter (TUI_KEY_NEWLINE) and pasted line breaks start a new line */
    int     scroll_row;
} TextField;

void  text_field_init(TextField *field, int max_chars);
/* Lets the text hold line breaks; off by default, when they become spaces. */
void  text_field_allow_newlines(TextField *field, int allow);
void  text_field_set(TextField *field, const char *utf8);
/* The text as UTF-8; the caller frees it. */
char *text_field_text(const TextField *field);
int   text_field_length(const TextField *field);
/* Returns 1 when the key was an edit or a cursor move the field used. */
int   text_field_key(TextField *field, int is_key_code, int ch);
void  text_field_paste(TextField *field, const char *utf8);
/* Draws into `rect`; when focused, reports where the cursor goes in `caret`. */
void  text_field_render(TextField *field, UiRect rect, int attr, int focused, TextCaret *caret);

#endif
