#include "managers/status_feed_manager.h"
#include "utilities/clock_util.h"
#include "utilities/path_util.h"
#include "utilities/str_util.h"

#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#define STATUS_CHAT        "status@broadcast"
#define PENDING_SLOTS      8
#define PENDING_EXPIRY_MS  (2 * 60 * 1000)
#define PRUNE_EVERY_MS     (10 * 60 * 1000)
#define MAX_AUTHORS        512

typedef struct Pending {
    char    id[64];
    int64_t until_ms;
} Pending;

struct StatusFeedManager {
    StatusFeedManagerDeps deps;
    IEventObserver        observer;
    char                  own_jid[128];
    Pending               pending[PENDING_SLOTS];
    int64_t               next_prune_ms;
    int                   changed;
    StatusLike            likes[8];       /* likes of your statuses not yet reported */
    int                   like_count;
};

#define FAR_FUTURE_S ((int64_t)1 << 40)

/* Statuses of the last day are current; older ones kept are the archive. */
static int64_t cutoff(void) { return (int64_t)time(NULL) - STATUS_FEED_LIFETIME_S; }

static int keep_days(const StatusFeedManager *m) {
    int days = m->deps.settings ? m->deps.settings->status_keep_days : 1;
    return days < 1 ? 1 : days;
}

static int64_t kept_since(const StatusFeedManager *m) { return (int64_t)time(NULL) - (int64_t)keep_days(m) * STATUS_FEED_LIFETIME_S; }

static void window(const StatusFeedManager *m, int archived, int64_t *since, int64_t *until) {
    *since = archived ? kept_since(m) : cutoff();
    *until = archived ? cutoff() : FAR_FUTURE_S;
}

static void clear_pending(StatusFeedManager *m, const char *id) {
    for (int i = 0; i < PENDING_SLOTS; i++) {
        if (strcmp(m->pending[i].id, id) == 0) m->pending[i].id[0] = '\0';
    }
}

static int is_pending(StatusFeedManager *m, const char *id, int64_t now) {
    for (int i = 0; i < PENDING_SLOTS; i++) {
        if (m->pending[i].id[0] && m->pending[i].until_ms > now && strcmp(m->pending[i].id, id) == 0) return 1;
    }
    return 0;
}

static void add_pending(StatusFeedManager *m, const char *id, int64_t now) {
    Pending *slot = &m->pending[0];
    for (int i = 0; i < PENDING_SLOTS; i++) {
        if (!m->pending[i].id[0] || m->pending[i].until_ms <= now) { slot = &m->pending[i]; break; }
        if (m->pending[i].until_ms < slot->until_ms) slot = &m->pending[i];   /* else the oldest gives way */
    }
    str_copy(slot->id, sizeof(slot->id), id);
    slot->until_ms = now + PENDING_EXPIRY_MS;
}

/* A file the backend names is used only when it is inside the media folder. */
static int trusted_file(StatusFeedManager *m, const char *path) {
    return path && path[0] && path_is_within(path, m->deps.media_dir) && path_is_regular_file(path);
}

static void download(StatusFeedManager *m, const StatusUpdate *u, int max_mb);

static int keep_status(StatusFeedManager *m, const Message *msg) {
    if (msg->timestamp <= kept_since(m)) return 0;
    StatusUpdate u;
    status_update_init(&u);
    str_copy(u.id, sizeof(u.id), msg->id);
    /* Your own statuses may come without a sender (Baileys names the chat). */
    const char *author = msg->sender_jid;
    if (msg->from_me && m->own_jid[0] && (!author[0] || strcmp(author, STATUS_CHAT) == 0)) author = m->own_jid;
    if (!author[0] || strcmp(author, STATUS_CHAT) == 0) return 0;
    str_copy(u.author_jid, sizeof(u.author_jid), author);
    str_copy(u.author_name, sizeof(u.author_name), msg->sender_name);
    u.type = msg->type;
    u.text = msg->text;                         /* borrowed for the save */
    u.media_ref = msg->media_ref;
    u.thumbnail = msg->thumbnail;
    u.thumbnail_len = msg->thumbnail_len;
    if (trusted_file(m, msg->media_path)) str_copy(u.media_path, sizeof(u.media_path), msg->media_path);
    u.background_argb = msg->background_argb;
    u.timestamp = msg->timestamp;
    u.from_me = msg->from_me;
    u.viewed = msg->from_me;                    /* you have seen what you posted */
    if (m->deps.store->save(m->deps.store, &u) != 0) return 0;
    /* With an archive, fetch the photo or video now: WhatsApp's copy does not last. */
    const Settings *s = m->deps.settings;
    if (keep_days(m) > 1 && s && s->auto_download_media && !u.media_path[0]) download(m, &u, s->auto_download_max_mb);
    return 1;
}

static int on_media(StatusFeedManager *m, const Event *e) {
    StatusUpdate u;
    if (m->deps.store->get(m->deps.store, e->id, &u) != 0) return 0;   /* not a status */
    status_update_dispose(&u);
    clear_pending(m, e->id);
    if (!trusted_file(m, e->path)) return 0;
    return m->deps.store->set_media_path(m->deps.store, e->id, e->path) == 0;
}

/* Someone liked (or unliked) one of your statuses. Likes of other people's
 * statuses never reach this account. */
static int on_like(StatusFeedManager *m, const Event *e) {
    StatusUpdate u;
    if (!m->deps.reactions || m->deps.store->get(m->deps.store, e->id, &u) != 0) return 0;
    int mine = u.from_me;
    status_update_dispose(&u);
    if (!mine) return 0;
    m->deps.reactions->put(m->deps.reactions, e->id, e->jid, e->emoji);
    if (e->emoji[0] && m->like_count < (int)(sizeof(m->likes) / sizeof(m->likes[0]))) {
        str_copy(m->likes[m->like_count].who, sizeof(m->likes[0].who), e->jid);
        str_copy(m->likes[m->like_count].emoji, sizeof(m->likes[0].emoji), e->emoji);
        m->like_count++;
    }
    return 1;
}

static int observe(IEventObserver *self, const Event *e) {
    StatusFeedManager *m = self->ctx;
    int used = 0;
    switch (e->type) {
        case EVENT_AUTH_CONNECTED:
            if (e->jid[0]) str_copy(m->own_jid, sizeof(m->own_jid), e->jid);
            return 0;
        case EVENT_MESSAGE_UPSERT:
            if (strcmp(e->message.chat_jid, STATUS_CHAT) == 0) used = keep_status(m, &e->message);
            break;
        case EVENT_MESSAGE_REMOVED:                   /* deleted for you on another device */
        case EVENT_MESSAGE_EDIT:
            if (strcmp(e->message.chat_jid, STATUS_CHAT) == 0 && (e->type == EVENT_MESSAGE_REMOVED || e->message.deleted)) {
                used = m->deps.store->remove(m->deps.store, e->message.id) == 0;
            }
            break;
        case EVENT_MEDIA_READY:
            used = on_media(m, e);
            break;
        case EVENT_REACTION:
            used = on_like(m, e);
            break;
        default:
            break;
    }
    if (used) m->changed = 1;
    return used;
}

static void observer_destroy(IEventObserver *self) { (void)self; }   /* owned by the manager */

StatusFeedManager *status_feed_manager_create(const StatusFeedManagerDeps *deps) {
    StatusFeedManager *m = calloc(1, sizeof(*m));
    if (!m) return NULL;
    m->deps = *deps;
    m->observer = (IEventObserver){ m, observe, observer_destroy };
    return m;
}

void status_feed_manager_destroy(StatusFeedManager *m) { free(m); }

IEventObserver *status_feed_manager_observer(StatusFeedManager *m) { return &m->observer; }

static void forget_file(void *ctx, const char *path) {
    StatusFeedManager *m = ctx;
    if (trusted_file(m, path)) unlink(path);
}

void status_feed_manager_tick(StatusFeedManager *m) {
    int64_t now = clock_now_ms();
    if (now < m->next_prune_ms) return;
    m->next_prune_ms = now + PRUNE_EVERY_MS;
    if (m->deps.store->prune(m->deps.store, kept_since(m), forget_file, m) > 0) m->changed = 1;
}

int status_feed_manager_authors(StatusFeedManager *m, int archived, StatusAuthor *out, int max) {
    int64_t since, until;
    window(m, archived, &since, &until);
    int n = m->deps.store->authors(m->deps.store, since, until, out, max);
    /* Yours first; the rest stay newest first. */
    for (int i = 0; i < n; i++) {
        if (!out[i].from_me || i == 0) continue;
        StatusAuthor mine = out[i];
        memmove(&out[1], &out[0], sizeof(out[0]) * (size_t)i);
        out[0] = mine;
        break;
    }
    return n;
}

int status_feed_manager_updates(StatusFeedManager *m, const char *jid, int archived, StatusUpdate *out, int max) {
    int64_t since, until;
    window(m, archived, &since, &until);
    return jid ? m->deps.store->updates_by(m->deps.store, jid, since, until, out, max) : 0;
}

void status_feed_manager_mark_viewed(StatusFeedManager *m, const char *id) {
    if (id && m->deps.store->mark_viewed(m->deps.store, id) == 0) m->changed = 1;
}

/* Asks for a status's photo or video unless it is here or on its way; `max_mb` 0 means no cap. */
static void download(StatusFeedManager *m, const StatusUpdate *u, int max_mb) {
    int64_t now = clock_now_ms();
    if (is_pending(m, u->id, now)) return;
    int wanted = (u->type == MESSAGE_TYPE_IMAGE || u->type == MESSAGE_TYPE_VIDEO) && u->media_ref &&
                 !(u->media_path[0] && path_is_regular_file(u->media_path));
    if (wanted && m->deps.gateway->download_media(m->deps.gateway, u->id, u->media_ref, max_mb) == 0) add_pending(m, u->id, now);
}

int status_feed_manager_get(StatusFeedManager *m, const char *id, StatusUpdate *out) {
    return id && id[0] ? m->deps.store->get(m->deps.store, id, out) : -1;
}

void status_feed_manager_fetch_media(StatusFeedManager *m, const char *id) {
    StatusUpdate u;
    if (!id || m->deps.store->get(m->deps.store, id, &u) != 0) return;
    download(m, &u, 0);                         /* no size cap: a status the user chose to look at */
    status_update_dispose(&u);
}

int status_feed_manager_unviewed_authors(StatusFeedManager *m) {
    StatusAuthor *authors = malloc(sizeof(*authors) * MAX_AUTHORS);
    if (!authors) return 0;
    int n = m->deps.store->authors(m->deps.store, cutoff(), FAR_FUTURE_S, authors, MAX_AUTHORS), count = 0;
    for (int i = 0; i < n; i++) count += authors[i].unviewed > 0 && !authors[i].from_me;
    free(authors);
    return count;
}

static int newest_first(const void *a, const void *b) {
    int64_t x = ((const StatusViewer *)a)->viewed_at, y = ((const StatusViewer *)b)->viewed_at;
    return x < y ? 1 : x > y ? -1 : 0;
}

int status_feed_manager_viewers(StatusFeedManager *m, const char *id, StatusViewer *out, int max) {
    if (!id || max <= 0) return 0;
    int n = 0;
    if (m->deps.receipts) {
        Receipt *seen = calloc((size_t)max, sizeof(*seen));
        int count = seen ? m->deps.receipts->list(m->deps.receipts, id, seen, max) : 0;
        for (int i = 0; i < count; i++) {
            int64_t at = seen[i].read_at ? seen[i].read_at : seen[i].played_at;
            if (!at) continue;                          /* delivered, not yet seen */
            memset(&out[n], 0, sizeof(out[n]));
            str_copy(out[n].jid, sizeof(out[n].jid), seen[i].jid);
            out[n++].viewed_at = at;
        }
        free(seen);
    }
    if (m->deps.reactions) {
        Reaction likes[64];
        int count = m->deps.reactions->list(m->deps.reactions, id, likes, 64);
        for (int i = 0; i < count; i++) {
            int k = 0;
            while (k < n && strcmp(out[k].jid, likes[i].sender) != 0) k++;
            if (k == n) {                               /* a like whose view receipt has not come */
                if (n == max) continue;
                memset(&out[n], 0, sizeof(out[n]));
                str_copy(out[n].jid, sizeof(out[n].jid), likes[i].sender);
                n++;
            }
            str_copy(out[k].reaction, sizeof(out[k].reaction), likes[i].emoji);
        }
    }
    qsort(out, (size_t)n, sizeof(*out), newest_first);
    return n;
}

int status_feed_manager_take_like(StatusFeedManager *m, StatusLike *out) {
    if (m->like_count == 0) return -1;
    *out = m->likes[0];
    memmove(&m->likes[0], &m->likes[1], sizeof(m->likes[0]) * (size_t)(m->like_count - 1));
    m->like_count--;
    return 0;
}

int status_feed_manager_take_changed(StatusFeedManager *m) {
    int changed = m->changed;
    m->changed = 0;
    return changed;
}
