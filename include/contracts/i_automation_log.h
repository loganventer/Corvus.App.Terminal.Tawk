#ifndef APP_CONTRACTS_I_AUTOMATION_LOG_H
#define APP_CONTRACTS_I_AUTOMATION_LOG_H

#include "core/automation_entry.h"

/* The record of what control socket clients did, newest kept. */
typedef struct IAutomationLog {
    void *ctx;
    int  (*append)(struct IAutomationLog *self, const AutomationEntry *entry);
    /* Newest first, up to `limit`; the caller frees `*out`. */
    int  (*recent)(struct IAutomationLog *self, int limit, AutomationEntry **out, int *count);
    void (*destroy)(struct IAutomationLog *self);
} IAutomationLog;

#endif
