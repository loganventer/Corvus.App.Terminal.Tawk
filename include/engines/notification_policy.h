#ifndef APP_ENGINES_NOTIFICATION_POLICY_H
#define APP_ENGINES_NOTIFICATION_POLICY_H

#include "core/chat.h"
#include "core/message.h"
#include "core/settings.h"

/* Decides whether an incoming message should alert the user. */
int notification_policy_should_notify(const Settings *settings, const Chat *chat,
                                      const Message *msg, int live, int chat_is_open);

#endif
