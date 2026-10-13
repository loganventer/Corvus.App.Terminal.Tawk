#ifndef APP_CLIENTS_TUI_RECEIPT_VIEW_H
#define APP_CLIENTS_TUI_RECEIPT_VIEW_H

#include "clients/tui/receipt_source.h"
#include "core/receipt_details.h"

/* How one voice note or video you sent was received, as the conversation
 * shows it: a head line that opens and closes ("▸ Played 2026-10-13 07:28:41"),
 * and under it, when open, one line each for Sent, Delivered, Read and
 * Played, with the date and the exact time. `found` is 0 when there is
 * nothing to show. */
typedef struct ReceiptView {
    int             found;
    ReceiptDetails *details;      /* owned */
} ReceiptView;

/* Looks the message's receipts up through `source` (which may be NULL). */
void receipt_view_load(ReceiptView *view, const ReceiptSource *source, const Message *message, int group, AccountId owner);
void receipt_view_dispose(ReceiptView *view);
/* The head line with its arrow, and how many lines show when it is open. */
const char *receipt_view_head(const ReceiptView *view, int open, char *out, int size);
int         receipt_view_line_count(const ReceiptView *view);
const char *receipt_view_line(const ReceiptView *view, int index);
void        receipt_view_draw_head(const ReceiptView *view, int open, int y, int x, int room, int attr);
void        receipt_view_draw_line(const ReceiptView *view, int index, int y, int x, int room, int attr);
/* The same rows for a soft-locked chat: their shape only. */
void        receipt_view_draw_veiled(const ReceiptView *view, int index, int y, int x, int room, int attr);

#endif
