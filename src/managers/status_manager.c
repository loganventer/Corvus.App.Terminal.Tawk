#include "managers/status_manager.h"
#include "engines/media_type_detector.h"
#include "engines/message_id_generator.h"
#include "engines/status_background_palette.h"
#include "engines/status_post_validator.h"
#include "utilities/clock_util.h"
#include "utilities/outgoing_media.h"
#include "utilities/str_util.h"

#include <stdlib.h>
#include <string.h>

#define ANSWER_TIMEOUT_MS 120000   /* a video can take a while to upload */
#define MAX_RESULTS       8

struct StatusManager {
    StatusManagerDeps deps;
    IEventObserver    observer;
    char              pending_id[64];     /* the post waiting for an answer, "" when none */
    int64_t           deadline_ms;
    StatusPostResult  results[MAX_RESULTS];
    int               result_count;
    char              error[256];
};

static void push_result(StatusManager *m, const char *id, int ok, const char *detail) {
    if (m->result_count >= MAX_RESULTS) {
        memmove(&m->results[0], &m->results[1], sizeof(m->results[0]) * (MAX_RESULTS - 1));
        m->result_count--;
    }
    StatusPostResult *r = &m->results[m->result_count++];
    str_copy(r->id, sizeof(r->id), id);
    r->ok = ok;
    str_copy(r->detail, sizeof(r->detail), ok ? "" : (detail && *detail ? detail : "The status could not be posted."));
}

static int refuse(StatusManager *m, const char *reason) {
    str_copy(m->error, sizeof(m->error), reason);
    return -1;
}

static int on_event(IEventObserver *self, const Event *e) {
    StatusManager *m = self->ctx;
    if (e->type != EVENT_STATUS_POSTED || !m->pending_id[0] || strcmp(e->id, m->pending_id) != 0) return 0;
    m->pending_id[0] = '\0';
    m->deadline_ms = 0;
    push_result(m, e->id, e->ok, e->detail);
    return 1;
}

static void observer_destroy(IEventObserver *self) { (void)self; }   /* owned by the manager */

StatusManager *status_manager_create(const StatusManagerDeps *deps) {
    StatusManager *m = calloc(1, sizeof(*m));
    if (!m) return NULL;
    m->deps = *deps;
    m->observer.ctx = m;
    m->observer.on_event = on_event;
    m->observer.destroy = observer_destroy;
    return m;
}

void status_manager_destroy(StatusManager *m) { free(m); }

IEventObserver *status_manager_observer(StatusManager *m) { return &m->observer; }

void status_manager_tick(StatusManager *m) {
    if (!m->pending_id[0] || clock_now_ms() < m->deadline_ms) return;
    push_result(m, m->pending_id, 0, "WhatsApp did not answer.");
    m->pending_id[0] = '\0';
    m->deadline_ms = 0;
}

int status_manager_supported(StatusManager *m) { return m->deps.publisher != NULL; }

int status_manager_post(StatusManager *m, const StatusPost *draft) {
    m->error[0] = '\0';
    if (!m->deps.publisher) return refuse(m, "Posting statuses needs the whatsmeow backend.");
    if (m->pending_id[0]) return refuse(m, "The last status is still being posted.");
    if (status_post_validate(draft, m->error, sizeof(m->error)) != 0) return -1;

    StatusPost *post = malloc(sizeof(*post));   /* large (text buffer), so off the stack */
    if (!post) return refuse(m, "Out of memory.");
    *post = *draft;
    if (message_id_generate(post->id, sizeof(post->id)) != 0) { free(post); return refuse(m, "Could not make a message id."); }
    if (status_kind_has_media(post->kind)) {
        char copy[600];
        if (outgoing_media_copy(m->deps.media_dir, draft->path, post->id, copy, sizeof(copy)) != 0) {
            free(post);
            return refuse(m, "The file could not be copied for sending.");
        }
        str_copy(post->path, sizeof(post->path), copy);
        str_copy(post->mime, sizeof(post->mime), media_type_mime(draft->path));
    } else {
        post->path[0] = post->mime[0] = '\0';
    }
    int rc = m->deps.publisher->post(m->deps.publisher, post);
    if (rc == 0) {
        str_copy(m->pending_id, sizeof(m->pending_id), post->id);
        m->deadline_ms = clock_now_ms() + ANSWER_TIMEOUT_MS;
    }
    free(post);
    return rc == 0 ? 0 : refuse(m, "The WhatsApp bridge did not take the status.");
}

const char *status_manager_error(StatusManager *m) { return m->error; }

StatusKind status_manager_kind_for_file(StatusManager *m, const char *path) {
    (void)m;
    switch (path && path[0] ? media_type_detect(path) : MESSAGE_TYPE_TEXT) {
        case MESSAGE_TYPE_IMAGE: return STATUS_KIND_PHOTO;
        case MESSAGE_TYPE_VIDEO: return STATUS_KIND_VIDEO;
        default:                 return STATUS_KIND_TEXT;
    }
}

int status_manager_background_count(StatusManager *m) { (void)m; return status_background_count(); }
uint32_t status_manager_background(StatusManager *m, int index) { (void)m; return status_background_at(index); }
const char *status_manager_background_name(StatusManager *m, int index) { (void)m; return status_background_name(index); }

int status_manager_busy(StatusManager *m) { return m->pending_id[0] != '\0'; }

int status_manager_take_result(StatusManager *m, StatusPostResult *out) {
    if (m->result_count == 0) return -1;
    *out = m->results[0];
    memmove(&m->results[0], &m->results[1], sizeof(m->results[0]) * (size_t)(m->result_count - 1));
    m->result_count--;
    return 0;
}
