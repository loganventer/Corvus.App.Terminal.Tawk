#include "resource_access/caching_message_store.h"
#include "utilities/lru_cache.h"

#include <stdlib.h>
#include <string.h>

typedef struct MessagePage {
    Message *items;
    int      count;
    int      limit;
} MessagePage;

typedef struct CachingMessages {
    IMessageStore *inner;
    LruCache      *pages;   /* chat jid -> MessagePage */
} CachingMessages;

static void free_page(void *value) {
    MessagePage *page = value;
    message_array_free(page->items, page->count);
    free(page);
}

static Message *copy_messages(const Message *items, int count) {
    Message *out = calloc((size_t)(count > 0 ? count : 1), sizeof(Message));
    if (!out) return NULL;
    for (int i = 0; i < count; i++) message_copy(&out[i], &items[i]);
    return out;
}

/* Status and media updates arrive by message id, so the owning chat is looked
 * up before invalidating. */
static void invalidate_by_id(CachingMessages *c, const char *id) {
    Message msg;
    if (c->inner->get(c->inner, id, &msg) == 0) {
        lru_cache_remove(c->pages, msg.chat_jid);
        message_dispose(&msg);
    }
}

static int cm_save(IMessageStore *self, const Message *msg) {
    CachingMessages *c = self->ctx;
    lru_cache_remove(c->pages, msg->chat_jid);
    return c->inner->save(c->inner, msg);
}

static int cm_recent(IMessageStore *self, const char *jid, int limit, Message **out, int *count) {
    CachingMessages *c = self->ctx;
    MessagePage *page = lru_cache_get(c->pages, jid);
    if (!page || page->limit != limit) {
        Message *items = NULL;
        int n = 0;
        if (c->inner->recent(c->inner, jid, limit, &items, &n) != 0) return -1;
        page = malloc(sizeof(*page));
        if (!page) { *out = items; *count = n; return 0; }
        *page = (MessagePage){ items, n, limit };
        lru_cache_put(c->pages, jid, page);
    }
    /* Callers own their result, so hand out a deep copy. */
    *out = copy_messages(page->items, page->count);
    *count = *out ? page->count : 0;
    return *out ? 0 : -1;
}

/* Older pages are read once while scrolling back, so they are not kept. */
static int cm_before(IMessageStore *self, const char *jid, int64_t before, int limit, Message **out, int *count) {
    CachingMessages *c = self->ctx;
    return c->inner->before(c->inner, jid, before, limit, out, count);
}

static int cm_get(IMessageStore *self, const char *id, Message *out) {
    CachingMessages *c = self->ctx;
    return c->inner->get(c->inner, id, out);
}

static int cm_update_status(IMessageStore *self, const char *id, MessageStatus status) {
    CachingMessages *c = self->ctx;
    invalidate_by_id(c, id);
    return c->inner->update_status(c->inner, id, status);
}

static int cm_set_media_path(IMessageStore *self, const char *id, const char *path) {
    CachingMessages *c = self->ctx;
    invalidate_by_id(c, id);
    return c->inner->set_media_path(c->inner, id, path);
}

static int cm_edit_text(IMessageStore *self, const char *id, const char *text, int deleted) {
    CachingMessages *c = self->ctx;
    invalidate_by_id(c, id);
    return c->inner->edit_text(c->inner, id, text, deleted);
}

static int cm_remove(IMessageStore *self, const char *id) {
    CachingMessages *c = self->ctx;
    invalidate_by_id(c, id);
    return c->inner->remove(c->inner, id);
}

static int cm_remove_chat(IMessageStore *self, const char *jid) {
    CachingMessages *c = self->ctx;
    lru_cache_remove(c->pages, jid);
    return c->inner->remove_chat(c->inner, jid);
}

static int cm_search(IMessageStore *self, const char *query, int limit, Message **out, int *count) {
    CachingMessages *c = self->ctx;
    return c->inner->search(c->inner, query, limit, out, count);
}

static int cm_reassign_jid(IMessageStore *self, const char *from, const char *to) {
    CachingMessages *c = self->ctx;
    lru_cache_clear(c->pages);
    return c->inner->reassign_jid(c->inner, from, to);
}

static void cm_destroy(IMessageStore *self) {
    if (!self) return;
    CachingMessages *c = self->ctx;
    lru_cache_destroy(c->pages);
    c->inner->destroy(c->inner);
    free(c);
    free(self);
}

IMessageStore *caching_message_store_create(IMessageStore *inner, int chats) {
    if (!inner) return NULL;
    IMessageStore *s = calloc(1, sizeof(*s));
    CachingMessages *c = calloc(1, sizeof(*c));
    if (!s || !c) { free(s); free(c); return inner; }
    c->inner = inner;
    c->pages = lru_cache_create(chats, free_page);
    if (!c->pages) { free(s); free(c); return inner; }
    s->ctx = c;
    s->save = cm_save;
    s->recent = cm_recent;
    s->before = cm_before;
    s->get = cm_get;
    s->update_status = cm_update_status;
    s->set_media_path = cm_set_media_path;
    s->reassign_jid = cm_reassign_jid;
    s->search = cm_search;
    s->edit_text = cm_edit_text;
    s->remove = cm_remove;
    s->remove_chat = cm_remove_chat;
    s->destroy = cm_destroy;
    return s;
}
