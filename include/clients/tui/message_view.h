#ifndef APP_CLIENTS_TUI_MESSAGE_VIEW_H
#define APP_CLIENTS_TUI_MESSAGE_VIEW_H

#include "clients/tui/image_placement.h"
#include "clients/tui/message_row.h"
#include "clients/tui/message_view_context.h"
#include "clients/tui/quoted_status.h"
#include "clients/tui/ui_rect.h"
#include "core/message.h"
#include "core/styled_text.h"

#define MESSAGE_VIEW_MAX_SCREEN_ROWS 512
#define MESSAGE_VIEW_MAX_PLACEMENTS  16

/* The conversation pane: bubbles, day separators, media and voice notes. */
typedef struct MessageView {
    int         scroll;          /* rows scrolled up from the newest message */
    int         selected;        /* message index, or -1 */
    int         follow_selection;  /* keep the selection on screen; off once the user scrolls */
    MessageRow *rows;
    int         row_count;
    int         row_cap;
    int         screen_rows[MESSAGE_VIEW_MAX_SCREEN_ROWS];   /* view row -> message index or -1 */
    unsigned char screen_quote[MESSAGE_VIEW_MAX_SCREEN_ROWS];  /* the row is a quote strip */
    short       screen_x0[MESSAGE_VIEW_MAX_SCREEN_ROWS];     /* the bubble's columns on that row */
    short       screen_x1[MESSAGE_VIEW_MAX_SCREEN_ROWS];
    UiRect      last_rect;
    UiRect      newer_button;    /* "↓ newer" badge, clickable when scrolled up */
    ImagePlacement placements[MESSAGE_VIEW_MAX_PLACEMENTS];   /* photos left blank for pixel images */
    int         header_rows;     /* the title bar: 1, or 2 with a portrait */
    UiRect      portrait_rect;   /* the title bar portrait, for clicks */
    UiRect      title_rect;      /* the name, for clicks */
    int         placement_count;
    StyledText *styled;          /* per message: its text as shown, formatted (text NULL when unformatted) */
    int         styled_count;
    QuotedStatus *quoted;        /* per message: the status it answers (found 0 when none) */
    int         quoted_count;
} MessageView;

void message_view_init(MessageView *view);
void message_view_dispose(MessageView *view);

void message_view_render(MessageView *view, UiRect rect, const Message *messages, int count,
                         const MessageViewContext *ctx);
void message_view_scroll(MessageView *view, int delta);
void message_view_scroll_to_latest(MessageView *view);
/* Moves the selection by delta messages (selection starts at the newest). */
void message_view_select(MessageView *view, int count, int delta);
/* Message whose bubble is under (y, x), or -1 (a click beside a bubble hits nothing). */
int  message_view_hit(MessageView *view, int y, int x);
/* The message whose quote strip is under (y, x), or -1. */
int  message_view_hit_quote(MessageView *view, int y, int x);
/* True when (y, x) is on the title bar portrait, or on the name. */
int  message_view_hit_portrait(const MessageView *view, int y, int x);
int  message_view_hit_title(const MessageView *view, int y, int x);
/* True when (y, x) is on the "↓ newer" badge. */
int  message_view_hit_newer(const MessageView *view, int y, int x);

#endif
