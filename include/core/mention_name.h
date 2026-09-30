#ifndef APP_CORE_MENTION_NAME_H
#define APP_CORE_MENTION_NAME_H

/* How a mention is shown: "@<user>" in the text becomes "@<name>". */
typedef struct MentionName {
    char user[64];
    char name[128];
} MentionName;

#endif
