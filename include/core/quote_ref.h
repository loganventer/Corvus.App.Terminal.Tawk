#ifndef APP_CORE_QUOTE_REF_H
#define APP_CORE_QUOTE_REF_H

/* The message a reply refers to. */
typedef struct QuoteRef {
    char id[64];
    char sender[128];
    char text[256];
    int  is_status;     /* the message answered is a status */
} QuoteRef;

#endif
