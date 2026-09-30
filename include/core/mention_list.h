#ifndef APP_CORE_MENTION_LIST_H
#define APP_CORE_MENTION_LIST_H

#include "core/mention.h"

#define MENTION_LIST_MAX 32

/* The people mentioned in one message. */
typedef struct MentionList {
    Mention items[MENTION_LIST_MAX];
    int     count;
} MentionList;

void mention_list_init(MentionList *list);
/* Adds a mention once; the user part defaults to the JID's user when empty. Returns -1 when full. */
int  mention_list_add(MentionList *list, const char *jid, const char *user);
const Mention *mention_list_find_user(const MentionList *list, const char *user);
/* To and from the stored form: one "jid\tuser" line per mention. */
char *mention_list_serialize(const MentionList *list);      /* caller frees; NULL when empty */
void  mention_list_parse(MentionList *list, const char *text);

#endif
