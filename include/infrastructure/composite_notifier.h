#ifndef APP_INFRASTRUCTURE_COMPOSITE_NOTIFIER_H
#define APP_INFRASTRUCTURE_COMPOSITE_NOTIFIER_H

#include "contracts/i_notifier.h"

#define COMPOSITE_NOTIFIER_MAX 8

/* Fans one notification out to several notifiers. Takes ownership of each
 * notifier added and destroys them with itself. */
INotifier *composite_notifier_create(void);
int        composite_notifier_add(INotifier *composite, INotifier *child);

#endif
