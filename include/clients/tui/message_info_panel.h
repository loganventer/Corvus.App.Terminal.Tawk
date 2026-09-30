#ifndef APP_CLIENTS_TUI_MESSAGE_INFO_PANEL_H
#define APP_CLIENTS_TUI_MESSAGE_INFO_PANEL_H

#include <stdint.h>

#include "clients/tui/popup_result.h"
#include "clients/tui/ui_rect.h"
#include "core/message.h"
#include "core/receipt.h"

/* Who received and read a message you sent, and when. The receipts are
 * handed in at each draw, so ones that arrive while it is open show up. */
typedef struct MessageInfoPanel {
    int     open;
    char    message_id[64];
    char    excerpt[160];       /* the start of the message, one line */
    int64_t sent_at;
    int     group;              /* sent to a group: list each member */
    int     scroll;
    UiRect  last_rect;
} MessageInfoPanel;

void        message_info_panel_open(MessageInfoPanel *panel, const Message *message, int group);
PopupResult message_info_panel_key(MessageInfoPanel *panel, int is_key_code, int ch);
PopupResult message_info_panel_click(MessageInfoPanel *panel, int y, int x);
void        message_info_panel_render(MessageInfoPanel *panel, UiRect area, const Receipt *receipts, int count,
                                      int use_24h);

#endif
