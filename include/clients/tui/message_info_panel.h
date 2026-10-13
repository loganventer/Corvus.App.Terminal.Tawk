#ifndef APP_CLIENTS_TUI_MESSAGE_INFO_PANEL_H
#define APP_CLIENTS_TUI_MESSAGE_INFO_PANEL_H

#include <stdint.h>

#include "clients/tui/popup_result.h"
#include "clients/tui/ui_rect.h"
#include "core/message.h"
#include "core/receipt.h"

#define MESSAGE_INFO_MAX_ITEMS 64
#define MESSAGE_INFO_MAX_ROWS  128

/* Who received, read and played a message you sent, and when. Each line with
 * a time has an arrow that opens it to the exact date and time, to the
 * second, and how long after the message it came. The receipts are handed in
 * at each draw, so ones that arrive while it is open show up. */
typedef struct MessageInfoPanel {
    int      open;
    char     message_id[64];
    char     excerpt[160];       /* the start of the message, one line */
    int64_t  sent_at;
    int      group;              /* sent to a group: list each member */
    int      playable;           /* a voice note or video you sent: the Played line always shows */
    int      scroll;
    int      cursor;             /* the line that opens which the arrow keys are on, among those at the last draw */
    int      item_count;         /* how many lines open, at the last draw */
    uint64_t opened;             /* bit k: line k is open */
    short    row_item[MESSAGE_INFO_MAX_ROWS];   /* the line that opens on each row of the last draw, or -1 */
    UiRect   last_rect;
} MessageInfoPanel;

void        message_info_panel_open(MessageInfoPanel *panel, const Message *message, int group);
PopupResult message_info_panel_key(MessageInfoPanel *panel, int is_key_code, int ch);
PopupResult message_info_panel_click(MessageInfoPanel *panel, int y, int x);
void        message_info_panel_render(MessageInfoPanel *panel, UiRect area, const Receipt *receipts, int count,
                                      int use_24h);

#endif
