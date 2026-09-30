#include "utilities/event_queue.h"

#include <pthread.h>
#include <stdlib.h>

struct EventQueue {
    Event          *items;
    int             capacity;
    int             head;
    int             count;
    int             closed;
    pthread_mutex_t mutex;
    pthread_cond_t  not_full;
};

EventQueue *event_queue_create(int capacity) {
    if (capacity <= 0) return NULL;
    EventQueue *q = calloc(1, sizeof(*q));
    if (!q) return NULL;
    q->items = calloc((size_t)capacity, sizeof(Event));
    if (!q->items) { free(q); return NULL; }
    q->capacity = capacity;
    pthread_mutex_init(&q->mutex, NULL);
    pthread_cond_init(&q->not_full, NULL);
    return q;
}

int event_queue_push(EventQueue *q, Event *evt) {
    pthread_mutex_lock(&q->mutex);
    while (q->count == q->capacity && !q->closed) {
        pthread_cond_wait(&q->not_full, &q->mutex);
    }
    if (q->closed) {
        pthread_mutex_unlock(&q->mutex);
        return -1;
    }
    q->items[(q->head + q->count) % q->capacity] = *evt;
    q->count++;
    event_init(evt, EVENT_NONE); /* ownership moved */
    pthread_mutex_unlock(&q->mutex);
    return 0;
}

int event_queue_pop(EventQueue *q, Event *out) {
    pthread_mutex_lock(&q->mutex);
    if (q->count == 0) {
        pthread_mutex_unlock(&q->mutex);
        return -1;
    }
    *out = q->items[q->head];
    q->head = (q->head + 1) % q->capacity;
    q->count--;
    pthread_cond_signal(&q->not_full);
    pthread_mutex_unlock(&q->mutex);
    return 0;
}

void event_queue_close(EventQueue *q) {
    pthread_mutex_lock(&q->mutex);
    q->closed = 1;
    pthread_cond_broadcast(&q->not_full);
    pthread_mutex_unlock(&q->mutex);
}

void event_queue_destroy(EventQueue *q) {
    if (!q) return;
    Event evt;
    while (event_queue_pop(q, &evt) == 0) event_dispose(&evt);
    pthread_mutex_destroy(&q->mutex);
    pthread_cond_destroy(&q->not_full);
    free(q->items);
    free(q);
}
