#ifndef APP_CORE_RECEIPT_KIND_H
#define APP_CORE_RECEIPT_KIND_H

/* What a receipt for a message you sent says. */
typedef enum ReceiptKind {
    RECEIPT_NONE = 0,
    RECEIPT_DELIVERED,         /* it reached their phone */
    RECEIPT_READ,              /* they opened it */
    RECEIPT_PLAYED             /* they played the voice note or video */
} ReceiptKind;

ReceiptKind receipt_kind_parse(const char *name);

#endif
