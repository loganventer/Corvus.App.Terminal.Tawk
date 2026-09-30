#ifndef APP_CORE_CONTACT_PROFILE_H
#define APP_CORE_CONTACT_PROFILE_H

#include <stdint.h>

/* What WhatsApp tells a linked device about a contact or a group, plus the
 * profile pictures tawk has downloaded for it. */
typedef struct ContactProfile {
    char    jid[128];
    char    about[700];              /* the contact's "about" text */
    char    verified_name[128];      /* business accounts */
    int     is_business;
    char    business_category[128];
    char    business_address[256];
    char    business_email[128];
    int     is_group;
    char    group_subject[128];
    char    group_description[1024];
    char    group_owner[128];
    int64_t group_created;
    int     participant_count;
    char   *participants;            /* "jid\t1\n" per member (1 = admin); owned */
    char    picture[600];            /* small preview, or "" */
    char    picture_full[600];       /* full size, or "" */
    int     picture_none;            /* WhatsApp says there is no picture (or it is private) */
    int     blocked;
    int64_t fetched_at;              /* when the details were last fetched (epoch seconds) */
} ContactProfile;

void contact_profile_init(ContactProfile *profile, const char *jid);
void contact_profile_dispose(ContactProfile *profile);
/* Deep copy (participants included). */
void contact_profile_copy(ContactProfile *dst, const ContactProfile *src);

#endif
