#include "core/receipt_kind.h"

#include <string.h>

ReceiptKind receipt_kind_parse(const char *name) {
    if (!name) return RECEIPT_NONE;
    if (strcmp(name, "delivered") == 0) return RECEIPT_DELIVERED;
    if (strcmp(name, "read") == 0) return RECEIPT_READ;
    if (strcmp(name, "played") == 0) return RECEIPT_PLAYED;
    return RECEIPT_NONE;
}
