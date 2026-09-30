#ifndef APP_CLIENTS_TUI_STATUS_FEED_REQUEST_H
#define APP_CLIENTS_TUI_STATUS_FEED_REQUEST_H

/* What the status feed dialogs need their owner to do after a key or click. */
typedef enum StatusFeedRequest {
    STATUS_FEED_NONE = 0,
    STATUS_FEED_REDRAW,
    STATUS_FEED_CLOSED,
    STATUS_FEED_OPEN_AUTHOR,   /* show status_feed_dialogs_author's statuses (call _view) */
    STATUS_FEED_SHOWING,       /* a status came into view: mark it seen, fetch its media */
    STATUS_FEED_OPEN_MEDIA,    /* open the shown photo or video full size */
    STATUS_FEED_POST,          /* start a new status */
    STATUS_FEED_OPEN_VIEWERS,  /* list who saw the status in view (only for your own) */
    STATUS_FEED_REPLY,         /* send status_feed_dialogs_reply_text to the status's author */
    STATUS_FEED_REACT,         /* send status_feed_dialogs_reaction to the status's author */
    STATUS_FEED_LIKE           /* like the status in view */
} StatusFeedRequest;

#endif
