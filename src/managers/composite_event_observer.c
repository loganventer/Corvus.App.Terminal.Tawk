#include "managers/composite_event_observer.h"

#include <stdlib.h>

#define MAX_OBSERVERS 8

typedef struct Composite {
    IEventObserver *items[MAX_OBSERVERS];
    int             count;
} Composite;

static int on_event(IEventObserver *self, const Event *e) {
    Composite *c = self->ctx;
    int changed = 0;
    for (int i = 0; i < c->count; i++) changed |= c->items[i]->on_event(c->items[i], e);
    return changed;
}

static void destroy(IEventObserver *self) {
    if (!self) return;
    free(self->ctx);
    free(self);
}

IEventObserver *composite_event_observer_create(void) {
    IEventObserver *o = calloc(1, sizeof(*o));
    Composite *c = calloc(1, sizeof(*c));
    if (!o || !c) { free(o); free(c); return NULL; }
    o->ctx = c;
    o->on_event = on_event;
    o->destroy = destroy;
    return o;
}

int composite_event_observer_add(IEventObserver *composite, IEventObserver *observer) {
    Composite *c = composite->ctx;
    if (!observer || c->count >= MAX_OBSERVERS) return -1;
    c->items[c->count++] = observer;
    return 0;
}
