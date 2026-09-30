#include "managers/call_manager.h"
#include "utilities/clock_util.h"
#include "utilities/str_util.h"

#include <stdlib.h>
#include <string.h>

#define RING_LIMIT_MS 60000     /* WhatsApp stops ringing after about a minute */

struct CallManager {
    IMessageGateway *gateway;
    IncomingCall     call;
    int              ringing;
    IEventObserver   observer;
};

static int on_event(IEventObserver *self, const Event *e) {
    CallManager *m = self->ctx;
    if (e->type != EVENT_CALL) return 0;
    if (strcmp(e->state, "offer") == 0) {
        if (e->video) return 0;                              /* video calls are not handled yet */
        memset(&m->call, 0, sizeof(m->call));
        str_copy(m->call.id, sizeof(m->call.id), e->id);
        str_copy(m->call.from, sizeof(m->call.from), e->jid);
        m->call.since_ms = clock_now_ms();
        m->ringing = 1;
        return 1;
    }
    if (m->ringing && strcmp(m->call.id, e->id) == 0) {     /* answered elsewhere, or over */
        m->ringing = 0;
        return 1;
    }
    return 0;
}

static void observer_destroy(IEventObserver *self) { (void)self; }

CallManager *call_manager_create(IMessageGateway *gateway) {
    CallManager *m = calloc(1, sizeof(*m));
    if (!m) return NULL;
    m->gateway = gateway;
    m->observer.ctx = m;
    m->observer.on_event = on_event;
    m->observer.destroy = observer_destroy;
    return m;
}

void call_manager_destroy(CallManager *m) { free(m); }

IEventObserver *call_manager_observer(CallManager *m) { return &m->observer; }

const IncomingCall *call_manager_ringing(CallManager *m) {
    if (m->ringing && clock_now_ms() - m->call.since_ms > RING_LIMIT_MS) m->ringing = 0;
    return m->ringing ? &m->call : NULL;
}

int call_manager_decline(CallManager *m) {
    if (!m->ringing) return -1;
    m->ringing = 0;
    return m->gateway->reject_call(m->gateway, m->call.from, m->call.id);
}

void call_manager_dismiss(CallManager *m) { m->ringing = 0; }
