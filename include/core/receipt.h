#ifndef APP_CORE_RECEIPT_H
#define APP_CORE_RECEIPT_H

#include <stdint.h>

/* How far one recipient got with a message you sent; 0 means not yet. */
typedef struct Receipt {
    char    jid[128];
    char    name[128];         /* filled by the manager for display */
    int64_t delivered_at;      /* epoch seconds */
    int64_t read_at;
    int64_t played_at;
} Receipt;

#endif
