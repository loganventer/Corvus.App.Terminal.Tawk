#ifndef APP_CORE_OUTGOING_TEXT_H
#define APP_CORE_OUTGOING_TEXT_H

#include "core/mention_list.h"
#include "core/quote_ref.h"

/* A text message on its way out, with what goes with it. */
typedef struct OutgoingText {
    const char        *text;
    const QuoteRef    *quote;              /* the message answered, or NULL */
    const MentionList *mentions;           /* people mentioned, or NULL */
    int                forwarded;
    int                forwarding_score;   /* how many times it has been forwarded */
    int                want_link_preview;  /* the backend may fetch a preview for its link */
} OutgoingText;

#endif
