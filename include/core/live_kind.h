#ifndef APP_CORE_LIVE_KIND_H
#define APP_CORE_LIVE_KIND_H

/* What just happened to a message, for readers that follow along. */
typedef enum LiveKind {
    LIVE_KIND_MESSAGE = 0,     /* it arrived, or you sent it */
    LIVE_KIND_READ             /* someone read one you sent */
} LiveKind;

#endif
