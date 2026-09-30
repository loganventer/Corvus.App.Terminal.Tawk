#ifndef APP_CLIENTS_TUI_STATUS_VIEWER_INTENT_H
#define APP_CLIENTS_TUI_STATUS_VIEWER_INTENT_H

/* What the user asked for about the status in view, beyond moving through them. */
typedef enum StatusViewerIntent {
    STATUS_VIEWER_INTENT_NONE = 0,
    STATUS_VIEWER_INTENT_VIEWERS,   /* who saw your own status */
    STATUS_VIEWER_INTENT_REPLY,     /* send the typed reply to its author */
    STATUS_VIEWER_INTENT_REACT,     /* send one of the quick emoji */
    STATUS_VIEWER_INTENT_LIKE       /* like it with a heart */
} StatusViewerIntent;

#endif
