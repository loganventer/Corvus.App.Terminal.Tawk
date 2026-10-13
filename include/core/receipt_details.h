#ifndef APP_CORE_RECEIPT_DETAILS_H
#define APP_CORE_RECEIPT_DETAILS_H

#define RECEIPT_DETAILS_MAX_LINES  20
#define RECEIPT_DETAILS_LINE_BYTES 160

/* What the conversation says about how a voice note or video you sent was
 * received: one head line that always shows, and the lines under it that show
 * when it is opened. `shown` is 0 when there is nothing to say. */
typedef struct ReceiptDetails {
    int  shown;
    int  played;                                              /* someone has played it */
    char head[RECEIPT_DETAILS_LINE_BYTES];                    /* "Played 2026-10-13 07:28:41" */
    int  line_count;
    char lines[RECEIPT_DETAILS_MAX_LINES][RECEIPT_DETAILS_LINE_BYTES];
} ReceiptDetails;

#endif
