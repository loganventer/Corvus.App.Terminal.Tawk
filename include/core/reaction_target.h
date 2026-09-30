#ifndef APP_CORE_REACTION_TARGET_H
#define APP_CORE_REACTION_TARGET_H

/* The message a reaction is attached to. */
typedef struct ReactionTarget {
    char chat[128];
    char id[64];
    char sender[128];
    int  from_me;
} ReactionTarget;

#endif
