#ifndef APP_CORE_PRIVACY_CHECK_H
#define APP_CORE_PRIVACY_CHECK_H

/* How a typed lock password fared. */
typedef enum {
    PRIVACY_CHECK_OK = 0,        /* it is the lock password */
    PRIVACY_CHECK_WRONG,         /* it is not; a wait may follow */
    PRIVACY_CHECK_WAIT,          /* too soon after wrong tries: it was not looked at */
    PRIVACY_CHECK_NO_PASSWORD    /* no lock password is set on this computer */
} PrivacyCheck;

#endif
