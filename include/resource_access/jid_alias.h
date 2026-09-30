#ifndef APP_RESOURCE_ACCESS_JID_ALIAS_H
#define APP_RESOURCE_ACCESS_JID_ALIAS_H

/* One alias -> canonical JID pair. */
typedef struct JidAlias {
    char alias[128];
    char canonical[128];
} JidAlias;

#endif
