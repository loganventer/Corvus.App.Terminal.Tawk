#ifndef APP_CORE_BIOMETRIC_RESULT_H
#define APP_CORE_BIOMETRIC_RESULT_H

/* How a Touch ID request ended, or that it has not ended yet. */
typedef enum {
    BIOMETRIC_PENDING = 0,    /* the system sheet is still up */
    BIOMETRIC_OK,             /* the fingerprint matched */
    BIOMETRIC_FALLBACK,       /* the user chose the tawk password instead */
    BIOMETRIC_CANCELLED,      /* the user, the system or tawk cancelled it */
    BIOMETRIC_FAILED,         /* the fingerprint did not match */
    BIOMETRIC_UNAVAILABLE     /* no Touch ID here: none fitted, none enrolled, locked out or no screen */
} BiometricResult;

#endif
