#ifndef APP_CLIENTS_TUI_SEARCH_OVERLAY_H
#define APP_CLIENTS_TUI_SEARCH_OVERLAY_H

#include "clients/tui/message_formatter.h"
#include "clients/tui/name_resolver.h"
#include "clients/tui/popup_result.h"
#include "clients/tui/text_caret.h"
#include "clients/tui/ui_rect.h"
#include "core/message.h"

#define SEARCH_OVERLAY_ROWS 128

/* Message search across every chat (Ctrl+K or /search). */
typedef struct SearchOverlay {
    int      open;
    char     query[128];
    Message *results;          /* owned */
    int      count;
    int      selected;
    int      scroll;
    int      row_item[SEARCH_OVERLAY_ROWS];
    UiRect   last_rect;
    TextCaret     caret;          /* the blinking cursor in the query field */
} SearchOverlay;

void           search_overlay_init(SearchOverlay *overlay);
void           search_overlay_open(SearchOverlay *overlay, const char *query);
void           search_overlay_close(SearchOverlay *overlay);
/* POPUP_CHANGED means the query changed and the caller should search again. */
PopupResult    search_overlay_key(SearchOverlay *overlay, int is_key_code, int ch);
PopupResult    search_overlay_click(SearchOverlay *overlay, int y, int x);
void           search_overlay_wheel(SearchOverlay *overlay, int delta);
/* Takes ownership of results (message_array_free'd later). */
void           search_overlay_set_results(SearchOverlay *overlay, Message *results, int count);
const Message *search_overlay_selected(const SearchOverlay *overlay);
/* `formatter` (may be NULL) gives each result's one-line text without formatting marks. */
void           search_overlay_render(SearchOverlay *overlay, UiRect area, const NameResolver *names,
                                     const MessageFormatter *formatter, int use_24h);

#endif
