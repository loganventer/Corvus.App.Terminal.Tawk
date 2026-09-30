#ifndef APP_CONTRACTS_I_CONTACT_STORE_H
#define APP_CONTRACTS_I_CONTACT_STORE_H

#include "core/contact.h"

typedef struct IContactStore {
    void *ctx;
    /* Merges: empty fields never overwrite known values. */
    int  (*upsert)(struct IContactStore *self, const Contact *contact);
    int  (*get)(struct IContactStore *self, const char *jid, Contact *out);
    /* Copies names known under `from` onto `to` where `to` has none. */
    int  (*merge)(struct IContactStore *self, const char *from, const char *to);
    void (*destroy)(struct IContactStore *self);
} IContactStore;

#endif
