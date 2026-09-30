#ifndef APP_MANAGERS_STATUS_MANAGER_H
#define APP_MANAGERS_STATUS_MANAGER_H

#include <stdint.h>

#include "contracts/i_event_observer.h"
#include "core/status_post.h"
#include "core/status_post_result.h"
#include "managers/status_manager_deps.h"

/* Posting your own statuses: checks the post, copies its photo or video
 * into the media folder, sends it and reports how it went. */
typedef struct StatusManager StatusManager;

StatusManager  *status_manager_create(const StatusManagerDeps *deps);
void            status_manager_destroy(StatusManager *mgr);
/* The observer to hand to the messaging manager (status_posted events). */
IEventObserver *status_manager_observer(StatusManager *mgr);
/* Gives up on posts the backend has not answered; call once per loop. */
void            status_manager_tick(StatusManager *mgr);

/* False when the running backend cannot post statuses (Baileys). */
int             status_manager_supported(StatusManager *mgr);
/* Returns 0 when the post was sent, or -1 with the reason in status_manager_error. */
int             status_manager_post(StatusManager *mgr, const StatusPost *post);
const char     *status_manager_error(StatusManager *mgr);
/* STATUS_KIND_PHOTO or STATUS_KIND_VIDEO for a file that can be posted as
 * one, else STATUS_KIND_TEXT (the file cannot be a status). */
StatusKind      status_manager_kind_for_file(StatusManager *mgr, const char *path);
/* The background colours offered for text statuses (index wraps around). */
int             status_manager_background_count(StatusManager *mgr);
uint32_t        status_manager_background(StatusManager *mgr, int index);
const char     *status_manager_background_name(StatusManager *mgr, int index);
int             status_manager_busy(StatusManager *mgr);
/* Hands over one finished post; returns 0 when there was one. */
int             status_manager_take_result(StatusManager *mgr, StatusPostResult *out);

#endif
