#ifndef APP_MANAGERS_STATUS_FEED_MANAGER_DEPS_H
#define APP_MANAGERS_STATUS_FEED_MANAGER_DEPS_H

#include "contracts/i_message_gateway.h"
#include "contracts/i_reaction_store.h"
#include "contracts/i_receipt_store.h"
#include "contracts/i_status_store.h"
#include "core/settings.h"

/* What the status feed manager depends on, injected by the composition root. */
typedef struct StatusFeedManagerDeps {
    IStatusStore    *store;
    IMessageGateway *gateway;      /* to download a status's photo or video */
    const char      *media_dir;
    const Settings  *settings;     /* how long to keep statuses, and the auto-download rules */
    IReceiptStore   *receipts;     /* who saw your statuses (their read receipts) */
    IReactionStore  *reactions;    /* who liked them */
} StatusFeedManagerDeps;

#endif
