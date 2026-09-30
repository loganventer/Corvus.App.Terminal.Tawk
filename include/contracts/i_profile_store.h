#ifndef APP_CONTRACTS_I_PROFILE_STORE_H
#define APP_CONTRACTS_I_PROFILE_STORE_H

#include "core/contact_profile.h"

/* Contact and group details and profile pictures, kept between runs. */
typedef struct IProfileStore {
    void *ctx;
    /* Fills `out` (caller disposes). Returns -1 when nothing is known. */
    int  (*get)(struct IProfileStore *self, const char *jid, ContactProfile *out);
    /* Saves the details (about, business, group), not the pictures or block state. */
    int  (*save_details)(struct IProfileStore *self, const ContactProfile *profile);
    /* A downloaded picture; `none` records that there is no picture. */
    int  (*set_picture)(struct IProfileStore *self, const char *jid, const char *path, int full, int none);
    /* The picture changed on WhatsApp: forget both sizes so they are fetched again. */
    int  (*forget_picture)(struct IProfileStore *self, const char *jid);
    /* Replaces the block list (JIDs separated by newlines). */
    int  (*set_blocklist)(struct IProfileStore *self, const char *jids);
    void (*destroy)(struct IProfileStore *self);
} IProfileStore;

#endif
