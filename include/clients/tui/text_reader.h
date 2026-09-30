#ifndef APP_CLIENTS_TUI_TEXT_READER_H
#define APP_CLIENTS_TUI_TEXT_READER_H

#include "clients/tui/popup_result.h"
#include "clients/tui/ui_rect.h"
#include "core/styled_text.h"

/* A full-screen, scrollable view of one long message (or help text). */
typedef struct TextReader {
    int    open;
    char  *text;          /* owned copy */
    StyledText styled;    /* the text formatted, when opened with _open_styled (text NULL otherwise) */
    char   title[160];
    int    scroll;
    int    total_rows;
    int    page_rows;
} TextReader;

void        text_reader_open(TextReader *reader, const char *title, const char *text);
/* A message's formatted text (copied); `styled` came from the message formatter. */
void        text_reader_open_styled(TextReader *reader, const char *title, const StyledText *styled);
void        text_reader_close(TextReader *reader);
PopupResult text_reader_key(TextReader *reader, int is_key_code, int ch);
void        text_reader_wheel(TextReader *reader, int delta);
void        text_reader_render(TextReader *reader, UiRect area);

#endif
