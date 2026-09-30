#ifndef APP_CORE_DELETE_REQUEST_H
#define APP_CORE_DELETE_REQUEST_H

#include <stdint.h>

/* A message to delete, for this account only or for everyone in the chat. */
typedef struct DeleteRequest {
    char    chat[128];
    char    id[64];
    char    sender[128];     /* groups: who sent it, when it was not us */
    int     from_me;
    int64_t timestamp;       /* when it was sent (delete for me needs it) */
    int     everyone;
} DeleteRequest;

#endif
