#ifndef APP_CLIENTS_TUI_STATUS_FEED_DIALOGS_H
#define APP_CLIENTS_TUI_STATUS_FEED_DIALOGS_H

#include <stdint.h>

#include "clients/tui/status_feed_request.h"
#include "clients/tui/status_list_dialog.h"
#include "clients/tui/status_viewer_dialog.h"
#include "clients/tui/status_viewers_dialog.h"

/* Looking at statuses, as one popup: the list of people with updates, and
 * the viewer over it for the person chosen. Esc in the viewer goes back to
 * the list. Knows nothing about where statuses come from. */
typedef struct StatusFeedDialogs {
    StatusListDialog   list;
    StatusViewerDialog viewer;
    StatusViewersDialog viewers;    /* who saw one of your statuses, over the viewer */
    int                chosen;      /* the author chosen in the list, as an index into the last authors */
} StatusFeedDialogs;

void status_feed_dialogs_open(StatusFeedDialogs *dialogs);
void status_feed_dialogs_close(StatusFeedDialogs *dialogs);
int  status_feed_dialogs_is_open(const StatusFeedDialogs *dialogs);
int  status_feed_dialogs_viewing(const StatusFeedDialogs *dialogs);
int  status_feed_dialogs_listing_viewers(const StatusFeedDialogs *dialogs);
/* After STATUS_FEED_OPEN_VIEWERS: shows the viewers list (the owner checked the status is yours). */
void status_feed_dialogs_show_viewers(StatusFeedDialogs *dialogs);

StatusFeedRequest status_feed_dialogs_key(StatusFeedDialogs *dialogs, int is_key_code, int ch);
StatusFeedRequest status_feed_dialogs_click(StatusFeedDialogs *dialogs, int y, int x);
void              status_feed_dialogs_wheel(StatusFeedDialogs *dialogs, int delta);

/* After STATUS_FEED_OPEN_AUTHOR: the index of the author chosen. */
int  status_feed_dialogs_author(const StatusFeedDialogs *dialogs);
/* Shows `author_jid`'s statuses from `start` of `count`. */
void status_feed_dialogs_view(StatusFeedDialogs *dialogs, const char *author_jid, const char *title, int start, int count);
/* The status in view, as an index into that author's statuses. */
int  status_feed_dialogs_index(const StatusFeedDialogs *dialogs);
/* After STATUS_FEED_REPLY: the reply typed (the caller frees it); after
 * STATUS_FEED_REACT: the emoji chosen. */
char       *status_feed_dialogs_reply_text(const StatusFeedDialogs *dialogs);
const char *status_feed_dialogs_reaction(const StatusFeedDialogs *dialogs);
/* The reply went out: the viewer stops taking typing. */
void        status_feed_dialogs_reply_sent(StatusFeedDialogs *dialogs);
/* Typing a reply (for the caret and for paste). */
int         status_feed_dialogs_replying(const StatusFeedDialogs *dialogs);
void        status_feed_dialogs_paste(StatusFeedDialogs *dialogs, const char *utf8);
/* Runs the viewer's timer, which steps to the next status by itself; `show_ms`
 * is how long the status in view stays and `hold` keeps it there. */
StatusFeedRequest status_feed_dialogs_tick(StatusFeedDialogs *dialogs, int64_t now_ms, int64_t show_ms, int hold);
/* True once after the viewer moved to another status. */
int  status_feed_dialogs_take_moved(StatusFeedDialogs *dialogs);

#endif
