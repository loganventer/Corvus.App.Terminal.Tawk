#include "core/contact_profile.h"
#include "utilities/str_util.h"

#include <stdlib.h>
#include <string.h>

void contact_profile_init(ContactProfile *p, const char *jid) {
    memset(p, 0, sizeof(*p));
    str_copy(p->jid, sizeof(p->jid), jid ? jid : "");
}

void contact_profile_dispose(ContactProfile *p) {
    if (!p) return;
    free(p->participants);
    p->participants = NULL;
}

void contact_profile_copy(ContactProfile *dst, const ContactProfile *src) {
    *dst = *src;
    dst->participants = src->participants ? strdup(src->participants) : NULL;
}
