#ifndef APP_CORE_AUTH_STATE_H
#define APP_CORE_AUTH_STATE_H

typedef enum AuthState {
    AUTH_STATE_STARTING = 0,   /* sidecar booting */
    AUTH_STATE_NEEDS_LOGIN,    /* not linked, waiting for QR scan or phone pairing */
    AUTH_STATE_CONNECTED,
    AUTH_STATE_RECONNECTING,
    AUTH_STATE_FAILED          /* sidecar could not start */
} AuthState;

#endif
