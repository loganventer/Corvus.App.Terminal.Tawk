#include "clients/tui/status_feed_dialogs.h"

#include <ncurses.h>
#include <string.h>

void status_feed_dialogs_open(StatusFeedDialogs *d) {
    memset(d, 0, sizeof(*d));
    d->chosen = -1;
    status_list_dialog_open(&d->list);
}

void status_feed_dialogs_close(StatusFeedDialogs *d) { d->list.open = d->viewer.open = d->viewers.open = 0; }
int  status_feed_dialogs_listing_viewers(const StatusFeedDialogs *d) { return status_feed_dialogs_viewing(d) && d->viewers.open; }
void status_feed_dialogs_show_viewers(StatusFeedDialogs *d) { status_viewers_dialog_open(&d->viewers); }

/* What the viewer was asked for (the viewers list, a reply, a reaction or
 * a like), passed on once; otherwise what the key did. */
static StatusFeedRequest viewers_wanted(StatusFeedDialogs *d, StatusFeedRequest otherwise) {
    StatusViewerIntent intent = d->viewer.intent;
    d->viewer.intent = STATUS_VIEWER_INTENT_NONE;
    switch (intent) {
        case STATUS_VIEWER_INTENT_VIEWERS: return STATUS_FEED_OPEN_VIEWERS;
        case STATUS_VIEWER_INTENT_REPLY:   return STATUS_FEED_REPLY;
        case STATUS_VIEWER_INTENT_REACT:   return STATUS_FEED_REACT;
        case STATUS_VIEWER_INTENT_LIKE:    return STATUS_FEED_LIKE;
        default:                           return otherwise;
    }
}

char *status_feed_dialogs_reply_text(const StatusFeedDialogs *d) { return status_viewer_dialog_reply_text(&d->viewer); }
const char *status_feed_dialogs_reaction(const StatusFeedDialogs *d) { return status_viewer_dialog_quick_emoji(d->viewer.intent_emoji); }
void status_feed_dialogs_reply_sent(StatusFeedDialogs *d) { d->viewer.replying = 0; }
int  status_feed_dialogs_replying(const StatusFeedDialogs *d) { return status_feed_dialogs_viewing(d) && d->viewer.replying; }
void status_feed_dialogs_paste(StatusFeedDialogs *d, const char *utf8) { status_viewer_dialog_paste(&d->viewer, utf8); }
int  status_feed_dialogs_is_open(const StatusFeedDialogs *d) { return d->list.open; }
int  status_feed_dialogs_viewing(const StatusFeedDialogs *d) { return d->list.open && d->viewer.open; }

static StatusFeedRequest from_list(StatusFeedDialogs *d, PopupResult r, int post) {
    if (post) return STATUS_FEED_POST;
    switch (r) {
        case POPUP_CHOSEN:
            d->chosen = status_list_dialog_choice(&d->list);
            return d->chosen >= 0 ? STATUS_FEED_OPEN_AUTHOR : STATUS_FEED_NONE;
        case POPUP_CLOSED:  status_feed_dialogs_close(d); return STATUS_FEED_CLOSED;
        case POPUP_CHANGED: return STATUS_FEED_REDRAW;
        default:            return STATUS_FEED_NONE;
    }
}

static StatusFeedRequest from_viewer(PopupResult r) {
    switch (r) {
        case POPUP_CHANGED: return STATUS_FEED_SHOWING;
        case POPUP_CHOSEN:  return STATUS_FEED_OPEN_MEDIA;
        case POPUP_CLOSED:  return STATUS_FEED_REDRAW;       /* back to the list */
        default:            return STATUS_FEED_NONE;
    }
}

StatusFeedRequest status_feed_dialogs_key(StatusFeedDialogs *d, int is_key, int ch) {
    if (d->viewer.open && d->viewers.open) {
        return status_viewers_dialog_key(&d->viewers, is_key, ch) == POPUP_NONE ? STATUS_FEED_NONE : STATUS_FEED_REDRAW;
    }
    if (d->viewer.open) return viewers_wanted(d, from_viewer(status_viewer_dialog_key(&d->viewer, is_key, ch)));
    int post = 0;
    PopupResult r = status_list_dialog_key(&d->list, is_key, ch, &post);
    return from_list(d, r, post);
}

StatusFeedRequest status_feed_dialogs_click(StatusFeedDialogs *d, int y, int x) {
    if (d->viewer.open && d->viewers.open) {
        return status_viewers_dialog_click(&d->viewers, y, x) == POPUP_NONE ? STATUS_FEED_NONE : STATUS_FEED_REDRAW;
    }
    if (d->viewer.open) return viewers_wanted(d, from_viewer(status_viewer_dialog_click(&d->viewer, y, x)));
    int post = 0;
    PopupResult r = status_list_dialog_click(&d->list, y, x, &post);
    return from_list(d, r, post);
}

void status_feed_dialogs_wheel(StatusFeedDialogs *d, int delta) {
    if (d->viewer.open && d->viewers.open) status_viewers_dialog_wheel(&d->viewers, delta);
    else if (d->viewer.open) status_viewer_dialog_key(&d->viewer, 1, delta < 0 ? KEY_LEFT : KEY_RIGHT);
    else status_list_dialog_wheel(&d->list, delta);
}

int  status_feed_dialogs_author(const StatusFeedDialogs *d) { return d->chosen; }

void status_feed_dialogs_view(StatusFeedDialogs *d, const char *jid, const char *title, int start, int count) {
    status_viewer_dialog_open(&d->viewer, jid, title, start, count);
}

int status_feed_dialogs_index(const StatusFeedDialogs *d) { return d->viewer.index; }

int status_feed_dialogs_take_moved(StatusFeedDialogs *d) {
    int moved = d->viewer.open && d->viewer.moved;
    d->viewer.moved = 0;
    return moved;
}
