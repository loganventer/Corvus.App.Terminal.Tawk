#include "core/control_inbound.h"

#include <stdlib.h>

void control_inbound_dispose(ControlInbound *inbound) {
    free(inbound->line);
    inbound->line = NULL;
}
