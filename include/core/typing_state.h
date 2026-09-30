#ifndef APP_CORE_TYPING_STATE_H
#define APP_CORE_TYPING_STATE_H

typedef enum TypingState {
    TYPING_PAUSED = 0,
    TYPING_COMPOSING,
    TYPING_RECORDING
} TypingState;

const char *typing_state_name(TypingState state);

#endif
