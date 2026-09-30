#include "infrastructure/composite_notifier.h"

#include <stdlib.h>

typedef struct Composite {
    INotifier *children[COMPOSITE_NOTIFIER_MAX];
    int        count;
} Composite;

static void composite_notify(INotifier *self, const Notification *n) {
    Composite *c = self->ctx;
    for (int i = 0; i < c->count; i++) c->children[i]->notify(c->children[i], n);
}

static void composite_destroy(INotifier *self) {
    if (!self) return;
    Composite *c = self->ctx;
    for (int i = 0; i < c->count; i++) c->children[i]->destroy(c->children[i]);
    free(c);
    free(self);
}

INotifier *composite_notifier_create(void) {
    INotifier *n = calloc(1, sizeof(*n));
    Composite *c = calloc(1, sizeof(*c));
    if (!n || !c) { free(n); free(c); return NULL; }
    n->ctx = c;
    n->notify = composite_notify;
    n->destroy = composite_destroy;
    return n;
}

int composite_notifier_add(INotifier *composite, INotifier *child) {
    Composite *c = composite->ctx;
    if (!child || c->count >= COMPOSITE_NOTIFIER_MAX) return -1;
    c->children[c->count++] = child;
    return 0;
}
