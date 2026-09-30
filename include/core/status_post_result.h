#ifndef APP_CORE_STATUS_POST_RESULT_H
#define APP_CORE_STATUS_POST_RESULT_H

/* How posting a status ended. */
typedef struct StatusPostResult {
    char id[64];
    int  ok;
    char detail[256];   /* why it failed; empty on success */
} StatusPostResult;

#endif
