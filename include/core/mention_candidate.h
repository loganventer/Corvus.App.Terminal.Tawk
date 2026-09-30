#ifndef APP_CORE_MENTION_CANDIDATE_H
#define APP_CORE_MENTION_CANDIDATE_H

/* A group member who can be mentioned, with the name shown for them. */
typedef struct MentionCandidate {
    char jid[128];
    char name[128];
} MentionCandidate;

#endif
