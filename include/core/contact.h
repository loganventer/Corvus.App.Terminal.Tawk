#ifndef APP_CORE_CONTACT_H
#define APP_CORE_CONTACT_H

typedef struct Contact {
    char jid[128];
    char name[128];       /* address-book name */
    char push_name[128];  /* name the contact chose */
} Contact;

void        contact_init(Contact *contact, const char *jid);
/* name, then push name, then "+<number>" from the JID. */
void        contact_display_name(const Contact *contact, char *out, unsigned long size);
/* "+27821234567" from "27821234567@s.whatsapp.net". */
void        contact_phone_from_jid(const char *jid, char *out, unsigned long size);

#endif
