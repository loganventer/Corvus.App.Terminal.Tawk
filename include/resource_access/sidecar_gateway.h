#ifndef APP_RESOURCE_ACCESS_SIDECAR_GATEWAY_H
#define APP_RESOURCE_ACCESS_SIDECAR_GATEWAY_H

#include "contracts/i_message_gateway.h"
#include "contracts/i_profile_editor.h"
#include "contracts/i_status_liker.h"
#include "resource_access/gateway_options.h"
#include "utilities/event_queue.h"

/* Runs the Baileys bridge as a Node.js child process and exchanges JSON
 * lines over its stdin/stdout. */
IMessageGateway *sidecar_gateway_create(const GatewayOptions *options, EventQueue *events);
/* A view of the same gateway for editing your profile; it lives as long as
 * the gateway. Baileys cannot post statuses here, so there is no publisher. */
IProfileEditor  *sidecar_gateway_profile_editor(IMessageGateway *gateway);
/* Status likes addressed to the author alone (whatsmeow cannot, and has none). */
IStatusLiker    *sidecar_gateway_status_liker(IMessageGateway *gateway);

#endif
