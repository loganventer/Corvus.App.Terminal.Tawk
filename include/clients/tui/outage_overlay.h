#ifndef APP_CLIENTS_TUI_OUTAGE_OVERLAY_H
#define APP_CLIENTS_TUI_OUTAGE_OVERLAY_H

#include "clients/tui/ui_rect.h"
#include "managers/connection_health.h"

/* A centred window that explains why WhatsApp is unavailable while the
 * supervisor backs off or the circuit breaker is open. R retries, Q quits. */
void outage_overlay_render(UiRect area, const ConnectionHealth *health);

#endif
