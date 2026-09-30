#ifndef APP_CONTRACTS_I_NOTIFIER_H
#define APP_CONTRACTS_I_NOTIFIER_H

#include "core/notification.h"

typedef struct INotifier {
    void *ctx;
    void (*notify)(struct INotifier *self, const Notification *notification);
    void (*destroy)(struct INotifier *self);
} INotifier;

#endif
