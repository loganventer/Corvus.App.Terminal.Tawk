#ifndef APP_MANAGERS_STATUS_FEED_MANAGER_H
#define APP_MANAGERS_STATUS_FEED_MANAGER_H

#include "contracts/i_event_observer.h"
#include "core/status_author.h"
#include "core/status_like.h"
#include "core/status_update.h"
#include "core/status_viewer.h"
#include "managers/status_feed_manager_deps.h"

#define STATUS_FEED_LIFETIME_S (24 * 60 * 60)

/* The statuses you can see: yours and your contacts'. WhatsApp shows a
 * status for a day; after that it moves to the archive here, kept for the
 * status_keep_days setting. Keeps what arrives on status@broadcast (live
 * and from history sync), fetches a status's photo or video when it is
 * looked at (or on arrival while the archive is kept, as WhatsApp's copy
 * does not last), remembers what you have seen, and forgets statuses and
 * their files once they are older than that. Viewing sends no read receipt. */
typedef struct StatusFeedManager StatusFeedManager;

StatusFeedManager *status_feed_manager_create(const StatusFeedManagerDeps *deps);
void               status_feed_manager_destroy(StatusFeedManager *mgr);
/* The observer to hand to the messaging manager (status messages, deletions, downloads). */
IEventObserver    *status_feed_manager_observer(StatusFeedManager *mgr);
/* Forgets expired statuses now and then; call once per loop. */
void               status_feed_manager_tick(StatusFeedManager *mgr);

/* Your own first (when you have any), then everyone else, newest first:
 * statuses of the last day, or with `archived` the older ones still kept. */
int  status_feed_manager_authors(StatusFeedManager *mgr, int archived, StatusAuthor *out, int max);
/* One author's statuses in the same window, oldest first; free with status_update_array_free. */
int  status_feed_manager_updates(StatusFeedManager *mgr, const char *jid, int archived, StatusUpdate *out, int max);
void status_feed_manager_mark_viewed(StatusFeedManager *mgr, const char *id);
/* One status by its id (to show the status a reply answers); returns -1
 * when it is not kept. The caller disposes `out`. */
int  status_feed_manager_get(StatusFeedManager *mgr, const char *id, StatusUpdate *out);
/* Asks for a status's photo or video unless it is here or on its way. */
void status_feed_manager_fetch_media(StatusFeedManager *mgr, const char *id);
/* How many people have statuses you have not seen (for the header). */
int  status_feed_manager_unviewed_authors(StatusFeedManager *mgr);
/* Who saw one of your statuses and who liked it, most recent first.
 * Returns how many were written. */
int  status_feed_manager_viewers(StatusFeedManager *mgr, const char *status_id, StatusViewer *out, int max);
/* Hands over one like of your statuses that just arrived; returns 0 when there was one. */
int  status_feed_manager_take_like(StatusFeedManager *mgr, StatusLike *out);
/* True once after anything in the feed changed. */
int  status_feed_manager_take_changed(StatusFeedManager *mgr);

#endif
