#include "managers/profile_manager.h"
#include "utilities/clock_util.h"
#include "utilities/path_util.h"
#include "utilities/str_util.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define CACHE_SLOTS        256
#define PENDING_SLOTS      64
#define REQUESTS_PER_TICK  2
#define PICTURE_STALE_S    (24 * 3600)
#define DETAILS_STALE_S    3600
#define WAIT_MS            20000

typedef enum { ASK_DETAILS = 0, ASK_PICTURE, ASK_FULL } Ask;

typedef struct Request {
    char    jid[128];
    Ask     ask;
    int     sent;
    int64_t sent_ms;
} Request;

typedef struct Slot {
    ContactProfile profile;
    int            loaded;        /* read from the store (or nothing there) */
    int            known;         /* the store had a row */
    int64_t        picture_asked; /* epoch seconds of the last preview request */
    int            refreshed;     /* details and picture were asked for since tawk started */
    int64_t        used;          /* for replacing the least recently used slot */
} Slot;

struct ProfileManager {
    ProfileManagerDeps deps;
    Slot               slots[CACHE_SLOTS];
    Request            pending[PENDING_SLOTS];
    int64_t            clock;
    IEventObserver     observer;
};

static Slot *slot_for(ProfileManager *m, const char *jid) {
    Slot *free_slot = NULL, *oldest = NULL;
    for (int i = 0; i < CACHE_SLOTS; i++) {
        Slot *s = &m->slots[i];
        if (s->loaded && strcmp(s->profile.jid, jid) == 0) { s->used = ++m->clock; return s; }
        if (!s->loaded && !free_slot) free_slot = s;
        if (!oldest || s->used < oldest->used) oldest = s;
    }
    Slot *s = free_slot ? free_slot : oldest;
    if (s->loaded) contact_profile_dispose(&s->profile);
    memset(s, 0, sizeof(*s));
    s->known = m->deps.store->get(m->deps.store, jid, &s->profile) == 0;
    if (!s->known) contact_profile_init(&s->profile, jid);
    s->loaded = 1;
    s->used = ++m->clock;
    return s;
}

static void ask(ProfileManager *m, const char *jid, Ask what) {
    Request *free_req = NULL;
    for (int i = 0; i < PENDING_SLOTS; i++) {
        Request *r = &m->pending[i];
        if (r->jid[0] && r->ask == what && strcmp(r->jid, jid) == 0) return;    /* already asked */
        if (!r->jid[0] && !free_req) free_req = r;
    }
    if (!free_req) return;                                   /* busy; asked again on a later frame */
    str_copy(free_req->jid, sizeof(free_req->jid), jid);
    free_req->ask = what;
    free_req->sent = 0;
}

static void answered(ProfileManager *m, const char *jid, Ask what) {
    for (int i = 0; i < PENDING_SLOTS; i++) {
        Request *r = &m->pending[i];
        if (r->jid[0] && r->ask == what && strcmp(r->jid, jid) == 0) r->jid[0] = '\0';
    }
}

void profile_manager_tick(ProfileManager *m) {
    int64_t now = clock_now_ms();
    int budget = REQUESTS_PER_TICK;
    for (int i = 0; i < PENDING_SLOTS; i++) {
        Request *r = &m->pending[i];
        if (!r->jid[0]) continue;
        if (r->sent) {
            if (now - r->sent_ms > WAIT_MS) r->jid[0] = '\0';   /* never answered: allow a later retry */
            continue;
        }
        if (budget-- <= 0) break;
        IMessageGateway *g = m->deps.gateway;
        if (r->ask == ASK_DETAILS) g->request_profile(g, r->jid);
        else g->request_picture(g, r->jid, r->ask == ASK_FULL);
        r->sent = 1;
        r->sent_ms = now;
    }
}

int profile_manager_busy(ProfileManager *m) {
    for (int i = 0; i < PENDING_SLOTS; i++) if (m->pending[i].jid[0]) return 1;
    return 0;
}

static int file_ok(const char *path) { return path && path[0] && path_is_regular_file(path); }

const char *profile_manager_picture(ProfileManager *m, const char *jid) {
    if (!jid || !jid[0]) return NULL;
    Slot *s = slot_for(m, jid);
    int64_t now = (int64_t)time(NULL);
    int have = file_ok(s->profile.picture);
    if (!s->refreshed) {                                     /* once per run: pick up changes since last time */
        s->refreshed = 1;
        s->picture_asked = now;
        ask(m, jid, ASK_PICTURE);
        ask(m, jid, ASK_DETAILS);
        return have ? s->profile.picture : NULL;
    }
    if ((!have && !s->profile.picture_none) || now - s->picture_asked > PICTURE_STALE_S) {
        if (now - s->picture_asked > 60) {                   /* at most once a minute per contact */
            s->picture_asked = now;
            ask(m, jid, ASK_PICTURE);
            if (!s->known) ask(m, jid, ASK_DETAILS);
        }
    }
    return have ? s->profile.picture : NULL;
}

const char *profile_manager_full_picture(ProfileManager *m, const char *jid) {
    Slot *s = slot_for(m, jid);
    if (file_ok(s->profile.picture_full)) return s->profile.picture_full;
    if (!s->profile.picture_none) ask(m, jid, ASK_FULL);
    return NULL;
}

int profile_manager_details(ProfileManager *m, const char *jid, int refresh, ContactProfile *out) {
    Slot *s = slot_for(m, jid);
    if (refresh || !s->known || (int64_t)time(NULL) - s->profile.fetched_at > DETAILS_STALE_S) ask(m, jid, ASK_DETAILS);
    contact_profile_copy(out, &s->profile);
    return s->known ? 0 : -1;
}

const char *profile_manager_summary(ProfileManager *m, const char *jid) {
    static char line[200];
    line[0] = '\0';
    if (!jid || !jid[0]) return line;
    const ContactProfile *p = &slot_for(m, jid)->profile;
    if (p->blocked) snprintf(line, sizeof(line), "Blocked");
    else if (p->is_group && p->participant_count) snprintf(line, sizeof(line), "%d members", p->participant_count);
    else if (p->about[0]) snprintf(line, sizeof(line), "%.190s", p->about);
    else if (p->is_business && p->business_category[0]) snprintf(line, sizeof(line), "%.190s", p->business_category);
    for (char *c = line; *c; c++) if (*c == '\n' || *c == '\r' || *c == '\t') *c = ' ';
    return line;
}

int profile_manager_is_blocked(ProfileManager *m, const char *jid) {
    return jid && jid[0] ? slot_for(m, jid)->profile.blocked : 0;
}

int profile_manager_set_blocked(ProfileManager *m, const char *jid, int blocked) {
    Slot *s = slot_for(m, jid);
    s->profile.blocked = blocked;                            /* shown at once; the block list confirms */
    return m->deps.gateway->set_blocked(m->deps.gateway, jid, blocked);
}

/* ---- events -------------------------------------------------------------- */

static void reload(ProfileManager *m, const char *whose) {
    char jid[128];
    str_copy(jid, sizeof(jid), whose);                       /* `whose` may be the jid of a slot disposed below */
    for (int i = 0; i < CACHE_SLOTS; i++) {
        Slot *s = &m->slots[i];
        if (!s->loaded || strcmp(s->profile.jid, jid) != 0) continue;
        int64_t asked = s->picture_asked;
        int refreshed = s->refreshed;
        contact_profile_dispose(&s->profile);
        s->known = m->deps.store->get(m->deps.store, jid, &s->profile) == 0;
        if (!s->known) contact_profile_init(&s->profile, jid);
        s->picture_asked = asked;
        s->refreshed = refreshed;
    }
}

static int on_event(IEventObserver *self, const Event *e) {
    ProfileManager *m = self->ctx;
    IProfileStore *store = m->deps.store;
    switch (e->type) {
        case EVENT_PROFILE: {
            if (!e->profile) return 0;
            ContactProfile p;
            contact_profile_copy(&p, e->profile);
            p.fetched_at = (int64_t)time(NULL);
            store->save_details(store, &p);
            answered(m, p.jid, ASK_DETAILS);
            reload(m, p.jid);
            contact_profile_dispose(&p);
            return 1;
        }
        case EVENT_PICTURE:
            /* only files the backend saved in the media folder */
            if (!e->path[0] || (m->deps.media_dir && path_is_within(e->path, m->deps.media_dir))) {
                store->set_picture(store, e->jid, e->path, e->full, e->none);
            }
            answered(m, e->jid, e->full ? ASK_FULL : ASK_PICTURE);
            reload(m, e->jid);
            return 1;
        case EVENT_PICTURE_CHANGED:
            store->forget_picture(store, e->jid);
            reload(m, e->jid);
            for (int i = 0; i < CACHE_SLOTS; i++) {
                if (m->slots[i].loaded && strcmp(m->slots[i].profile.jid, e->jid) == 0) m->slots[i].picture_asked = 0;
            }
            return 1;
        case EVENT_BLOCKLIST:
            store->set_blocklist(store, e->list ? e->list : "");
            for (int i = 0; i < CACHE_SLOTS; i++) if (m->slots[i].loaded) reload(m, m->slots[i].profile.jid);
            return 1;
        default:
            return 0;
    }
}

static void observer_destroy(IEventObserver *self) { (void)self; }   /* owned by the manager */

IEventObserver *profile_manager_observer(ProfileManager *m) { return &m->observer; }

ProfileManager *profile_manager_create(const ProfileManagerDeps *deps) {
    ProfileManager *m = calloc(1, sizeof(*m));
    if (!m) return NULL;
    m->deps = *deps;
    m->observer.ctx = m;
    m->observer.on_event = on_event;
    m->observer.destroy = observer_destroy;
    return m;
}

void profile_manager_destroy(ProfileManager *m) {
    if (!m) return;
    for (int i = 0; i < CACHE_SLOTS; i++) if (m->slots[i].loaded) contact_profile_dispose(&m->slots[i].profile);
    free(m);
}
