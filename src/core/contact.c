#include "core/contact.h"
#include "utilities/str_util.h"

#include <stdio.h>
#include <string.h>

void contact_init(Contact *contact, const char *jid) {
    memset(contact, 0, sizeof(*contact));
    str_copy(contact->jid, sizeof(contact->jid), jid);
}

void contact_phone_from_jid(const char *jid, char *out, unsigned long size) {
    char digits[64] = {0};
    size_t n = 0;
    for (const char *p = jid; p && *p && *p != '@' && *p != ':' && n < sizeof(digits) - 1; p++) {
        digits[n++] = *p;
    }
    snprintf(out, size, "+%s", digits);
}

void contact_display_name(const Contact *contact, char *out, unsigned long size) {
    if (contact->name[0]) str_copy(out, size, contact->name);
    else if (contact->push_name[0]) str_copy(out, size, contact->push_name);
    else contact_phone_from_jid(contact->jid, out, size);
}
