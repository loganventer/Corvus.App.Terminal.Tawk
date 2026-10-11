#ifndef APP_CORE_SUMMARY_DECLINE_RESULT_H
#define APP_CORE_SUMMARY_DECLINE_RESULT_H

/* What became of an agent's answer that a message needs no summary. */
typedef enum SummaryDeclineResult {
    SUMMARY_DECLINED = 0,
    SUMMARY_DECLINE_OFF,       /* this chat is not in TL;DR mode */
    SUMMARY_DECLINE_REFUSED,   /* not a text message, or the reason is not one tawk knows */
    SUMMARY_DECLINE_FAILED     /* it could not be kept */
} SummaryDeclineResult;

#endif
