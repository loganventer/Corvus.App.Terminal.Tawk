#ifndef APP_RESOURCE_ACCESS_WHATSMEOW_GATEWAY_H
#define APP_RESOURCE_ACCESS_WHATSMEOW_GATEWAY_H

#include "contracts/i_message_gateway.h"
#include "contracts/i_profile_editor.h"
#include "contracts/i_status_publisher.h"
#include "resource_access/gateway_options.h"
#include "utilities/event_queue.h"

/* Runs the whatsmeow (Go) protocol library in-process. Returns NULL when
 * the binary was built without it (make WHATSMEOW=0). */
IMessageGateway *whatsmeow_gateway_create(const GatewayOptions *options, EventQueue *events);
/* Views of the same gateway for editing your profile and posting statuses;
 * they live as long as the gateway. */
IProfileEditor   *whatsmeow_gateway_profile_editor(IMessageGateway *gateway);
IStatusPublisher *whatsmeow_gateway_status_publisher(IMessageGateway *gateway);

#endif
