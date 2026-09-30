#ifndef APP_MANAGERS_CALL_MANAGER_H
#define APP_MANAGERS_CALL_MANAGER_H

#include "contracts/i_event_observer.h"
#include "contracts/i_message_gateway.h"
#include "core/incoming_call.h"

/* Incoming voice calls: which one is ringing, declining it, or dismissing
 * the prompt to answer on the phone. Video calls are ignored for now.
 * tawk cannot carry call audio, so it never answers. */
typedef struct CallManager CallManager;

CallManager    *call_manager_create(IMessageGateway *gateway);
void            call_manager_destroy(CallManager *mgr);
IEventObserver *call_manager_observer(CallManager *mgr);
/* The ringing call, or NULL. */
const IncomingCall *call_manager_ringing(CallManager *mgr);
/* Declines the ringing call. */
int             call_manager_decline(CallManager *mgr);
/* Stops showing the ringing call (it can still be answered on the phone). */
void            call_manager_dismiss(CallManager *mgr);

#endif
