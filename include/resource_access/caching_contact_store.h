#ifndef APP_RESOURCE_ACCESS_CACHING_CONTACT_STORE_H
#define APP_RESOURCE_ACCESS_CACHING_CONTACT_STORE_H

#include "contracts/i_contact_store.h"

/* Decorator: keeps recently used contacts in memory in front of another
 * IContactStore. Takes ownership of `inner`. */
IContactStore *caching_contact_store_create(IContactStore *inner, int capacity);

#endif
