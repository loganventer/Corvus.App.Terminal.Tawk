#include "engines/circuit_state.h"

const char *circuit_state_name(CircuitState state) {
    switch (state) {
        case CIRCUIT_CLOSED:    return "closed";
        case CIRCUIT_OPEN:      return "open";
        case CIRCUIT_HALF_OPEN: return "half-open";
    }
    return "?";
}
