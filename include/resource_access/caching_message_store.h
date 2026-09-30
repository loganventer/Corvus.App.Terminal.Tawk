#ifndef APP_RESOURCE_ACCESS_CACHING_MESSAGE_STORE_H
#define APP_RESOURCE_ACCESS_CACHING_MESSAGE_STORE_H

#include "contracts/i_message_store.h"

/* Decorator: caches the latest page of messages for recently opened chats in
 * front of another IMessageStore. Any write to a chat invalidates its page.
 * Takes ownership of `inner`. */
IMessageStore *caching_message_store_create(IMessageStore *inner, int chats);

#endif
