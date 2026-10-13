#include "clients/tui/receipt_view.h"
#include "clients/tui/text_veil.h"
#include "clients/tui/tui_draw.h"
#include "clients/tui/tui_palette.h"
#include "engines/receipt_details_builder.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_RECEIPTS 256
#define ARROW_CLOSED "\xE2\x96\xB8 "      /* ▸ */
#define ARROW_OPEN   "\xE2\x96\xBE "      /* ▾ */
#define INDENT       "  "

void receipt_view_load(ReceiptView *v, const ReceiptSource *source, const Message *message, int group, AccountId owner) {
    memset(v, 0, sizeof(*v));
    if (!source || !source->list || !message || !message->from_me) return;
    static Receipt receipts[MAX_RECEIPTS];          /* the terminal draws on one thread: one buffer serves every note */
    ReceiptDetails *details = calloc(1, sizeof(ReceiptDetails));
    if (!details) return;
    int count = source->list(source->ctx, message, owner, receipts, MAX_RECEIPTS);
    if (count < 0) count = 0;
    if (count > MAX_RECEIPTS) count = MAX_RECEIPTS;
    receipt_details_build(message, receipts, count, group, details);
    if (!details->shown) { free(details); return; }
    v->details = details;
    v->found = 1;
}

void receipt_view_dispose(ReceiptView *v) {
    free(v->details);
    memset(v, 0, sizeof(*v));
}

const char *receipt_view_head(const ReceiptView *v, int open, char *out, int size) {
    if (!v->found) { if (size > 0) out[0] = '\0'; return out; }
    snprintf(out, (size_t)size, "%s%s", open ? ARROW_OPEN : ARROW_CLOSED, v->details->head);
    return out;
}

int receipt_view_line_count(const ReceiptView *v) { return v->found ? v->details->line_count : 0; }

const char *receipt_view_line(const ReceiptView *v, int index) {
    return v->found && index >= 0 && index < v->details->line_count ? v->details->lines[index] : "";
}

void receipt_view_draw_head(const ReceiptView *v, int open, int y, int x, int room, int attr) {
    char head[RECEIPT_DETAILS_LINE_BYTES + 8];
    receipt_view_head(v, open, head, sizeof(head));
    tui_text(y, x, room, head, attr | ATTR_DIM | ATTR_BOLD);
}

void receipt_view_draw_line(const ReceiptView *v, int index, int y, int x, int room, int attr) {
    char line[RECEIPT_DETAILS_LINE_BYTES + 8];
    snprintf(line, sizeof(line), INDENT "%s", receipt_view_line(v, index));
    tui_text(y, x, room, line, attr | ATTR_DIM);
}

void receipt_view_draw_veiled(const ReceiptView *v, int index, int y, int x, int room, int attr) {
    const char *text = index < 0 ? v->details->head : receipt_view_line(v, index);
    text_veil_text(y, x, room, text, strlen(text), attr);
}
