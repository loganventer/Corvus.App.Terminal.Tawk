#include "core/typing_state.h"

const char *typing_state_name(TypingState state) {
    switch (state) {
        case TYPING_COMPOSING: return "composing";
        case TYPING_RECORDING: return "recording";
        default:               return "paused";
    }
}
