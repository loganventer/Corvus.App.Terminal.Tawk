#ifndef APP_CORE_REACTION_H
#define APP_CORE_REACTION_H

/* One person's reaction to a message. */
typedef struct Reaction {
    char sender[128];
    char emoji[32];
} Reaction;

#endif
