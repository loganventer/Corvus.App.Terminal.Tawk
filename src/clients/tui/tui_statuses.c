/* Looking at statuses: the feed dialogs, carried out through the status
 * feed manager. */
#include "tui_app_state.h"
#include "utilities/str_util.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_AUTHORS  STATUS_LIST_ROWS
#define MAX_UPDATES  64

static const Settings *settings(TuiApp *app) { return settings_manager_current(app->deps.settings); }

/* A contact's saved name when there is one, else the push name the status came with. */
static const char *author_name(void *ctx, const StatusAuthor *author) {
    static char name[128];
    TuiApp *app = ctx;
    messaging_manager_display_name(app->deps.messaging, author->jid, name, sizeof(name));
    if ((!name[0] || strcmp(name, author->jid) == 0 || name[0] == '+') && author->name[0]) return author->name;
    return name;
}

static void viewer_name(void *ctx, const char *jid, char *out, size_t size) {
    messaging_manager_display_name(((TuiApp *)ctx)->deps.messaging, jid, out, size);
}

void tui_app_open_statuses(TuiApp *app) {
    if (tui_app_show_login(app) || !app->deps.feed) return;
    status_feed_dialogs_open(&app->feed);
    app->dirty = 1;
}

/* The chosen author's statuses start at the first one not yet seen. */
static void open_author(TuiApp *app) {
    StatusAuthor authors[MAX_AUTHORS];
    int n = status_feed_manager_authors(app->deps.feed, app->feed.list.archived, authors, MAX_AUTHORS);
    int chosen = status_feed_dialogs_author(&app->feed);
    if (chosen < 0 || chosen >= n) return;
    StatusUpdate items[MAX_UPDATES];
    int count = status_feed_manager_updates(app->deps.feed, authors[chosen].jid, app->feed.list.archived, items, MAX_UPDATES);
    int start = 0;
    while (!authors[chosen].from_me && start < count - 1 && items[start].viewed) start++;
    status_update_array_free(items, count);
    const char *title = authors[chosen].from_me ? "My status" : author_name(app, &authors[chosen]);
    status_feed_dialogs_view(&app->feed, authors[chosen].jid, title, start, count);
}

/* The status in view, copied into `out` (the caller disposes it). */
static int current(TuiApp *app, StatusUpdate *out) {
    StatusUpdate items[MAX_UPDATES];
    int count = status_feed_manager_updates(app->deps.feed, app->feed.viewer.author_jid, app->feed.list.archived, items, MAX_UPDATES);
    int index = status_feed_dialogs_index(&app->feed);
    int found = index >= 0 && index < count;
    if (found) {
        *out = items[index];
        status_update_init(&items[index]);          /* ownership moves to `out` */
    }
    status_update_array_free(items, count);
    return found ? 0 : -1;
}

static void open_media(TuiApp *app) {
    StatusUpdate u;
    if (current(app, &u) != 0) return;
    if (u.type != MESSAGE_TYPE_IMAGE && u.type != MESSAGE_TYPE_VIDEO) {
        status_update_dispose(&u);
        return;
    }
    if (!u.media_path[0]) {
        status_feed_manager_fetch_media(app->deps.feed, u.id);
        tui_app_toast(app, "Still downloading; try again in a moment", 0);
    } else if (u.type == MESSAGE_TYPE_IMAGE && (!settings(app)->image_viewer[0] || strcmp(settings(app)->image_viewer, "builtin") == 0)) {
        image_viewer_open_portrait(&app->viewer, u.author_jid, app->feed.viewer.title, u.media_path);
    } else if (media_manager_activate(app->deps.media, u.media_path, u.type) != 0) {
        tui_app_toast(app, "Could not open it", 1);
    }
    status_update_dispose(&u);
}

/* The viewers list, only for a status of yours. */
static void open_viewers(TuiApp *app) {
    StatusUpdate u;
    if (current(app, &u) != 0) return;
    if (u.from_me) status_feed_dialogs_show_viewers(&app->feed);
    status_update_dispose(&u);
}

/* What a reply quotes: the status in view, whose it is and its words (or its kind). */
static int reply_target(TuiApp *app, StatusReplyTarget *t) {
    StatusUpdate u;
    if (current(app, &u) != 0) return -1;
    memset(t, 0, sizeof(*t));
    str_copy(t->status_id, sizeof(t->status_id), u.id);
    str_copy(t->author_jid, sizeof(t->author_jid), u.author_jid);
    const char *kind = u.type == MESSAGE_TYPE_IMAGE ? "\xF0\x9F\x93\xB7 Photo" : u.type == MESSAGE_TYPE_VIDEO ? "\xF0\x9F\x8E\xAC Video" : "";
    str_copy(t->preview, sizeof(t->preview), u.text && u.text[0] ? u.text : kind);
    int mine = u.from_me;
    status_update_dispose(&u);
    return mine ? -1 : 0;                                /* your own statuses take no answers */
}

static void answer(TuiApp *app, StatusFeedRequest request) {
    StatusReplyTarget t;
    if (reply_target(app, &t) != 0) return;
    char name[128], msg[256];
    messaging_manager_display_name(app->deps.messaging, t.author_jid, name, sizeof(name));
    if (request == STATUS_FEED_LIKE) {
        int how = messaging_manager_like_status(app->deps.messaging, &t);
        if (how == 0) snprintf(msg, sizeof(msg), "\xE2\x9D\xA4\xEF\xB8\x8F Liked %s's status", name);
        else if (how == 1) snprintf(msg, sizeof(msg), "\xE2\x9D\xA4\xEF\xB8\x8F sent to %s as a reply", name);
        else snprintf(msg, sizeof(msg), "Could not like the status");
        tui_app_toast(app, msg, how < 0);
        return;
    }
    char *text = request == STATUS_FEED_REPLY ? status_feed_dialogs_reply_text(&app->feed) : NULL;
    const char *body = text ? str_trim(text) : status_feed_dialogs_reaction(&app->feed);
    int rc = body && *body ? messaging_manager_reply_to_status(app->deps.messaging, &t, body) : -1;
    free(text);
    if (rc == 0) {
        status_feed_dialogs_reply_sent(&app->feed);
        snprintf(msg, sizeof(msg), "Sent to %s", name);
    } else {
        snprintf(msg, sizeof(msg), "Could not send the reply");
    }
    tui_app_toast(app, msg, rc != 0);
}

void tui_app_statuses_request(TuiApp *app, StatusFeedRequest request) {
    switch (request) {
        case STATUS_FEED_REPLY:
        case STATUS_FEED_REACT:
        case STATUS_FEED_LIKE:         answer(app, request); break;
        case STATUS_FEED_OPEN_VIEWERS: open_viewers(app); break;
        case STATUS_FEED_OPEN_AUTHOR: open_author(app); break;
        case STATUS_FEED_OPEN_MEDIA:  open_media(app); break;
        case STATUS_FEED_POST:
            status_feed_dialogs_close(&app->feed);
            tui_app_open_status(app);
            break;
        case STATUS_FEED_NONE:        return;
        default:                      break;
    }
    app->dirty = 1;
}

/* The status that just came into view counts as seen, and its photo or
 * video is fetched. Nothing is sent back to its author. */
void tui_app_statuses_tick(TuiApp *app) {
    if (!app->deps.feed) return;
    status_feed_manager_tick(app->deps.feed);
    if (status_feed_manager_take_changed(app->deps.feed)) app->dirty = 1;
    StatusLike like;
    while (status_feed_manager_take_like(app->deps.feed, &like) == 0) {
        char name[128], msg[256];
        messaging_manager_display_name(app->deps.messaging, like.who, name, sizeof(name));
        snprintf(msg, sizeof(msg), "%s %s liked your status", like.emoji, name);
        tui_app_toast(app, msg, 0);
        app->dirty = 1;
    }
    if (!status_feed_dialogs_take_moved(&app->feed)) return;
    StatusUpdate u;
    if (current(app, &u) != 0) return;
    if (!u.viewed) status_feed_manager_mark_viewed(app->deps.feed, u.id);
    if ((u.type == MESSAGE_TYPE_IMAGE || u.type == MESSAGE_TYPE_VIDEO) && !u.media_path[0]) status_feed_manager_fetch_media(app->deps.feed, u.id);
    status_update_dispose(&u);
    app->dirty = 1;
}

void tui_app_statuses_render(TuiApp *app, UiRect area) {
    if (status_feed_dialogs_viewing(&app->feed)) {
        StatusUpdate items[MAX_UPDATES];
        int count = status_feed_manager_updates(app->deps.feed, app->feed.viewer.author_jid, app->feed.list.archived, items, MAX_UPDATES);
        int index = status_feed_dialogs_index(&app->feed);
        const StatusUpdate *shown = index >= 0 && index < count ? &items[index] : NULL;
        StatusViewer viewers[STATUS_VIEWERS_ROWS];
        int seen = shown && shown->from_me ? status_feed_manager_viewers(app->deps.feed, shown->id, viewers, STATUS_VIEWERS_ROWS) : 0;
        char label[64] = "";
        if (shown && shown->from_me) {
            int likes = 0;
            for (int i = 0; i < seen; i++) likes += viewers[i].reaction[0] != '\0';
            if (likes) snprintf(label, sizeof(label), "Seen by %d \xC2\xB7 \xE2\x9D\xA4\xEF\xB8\x8F %d", seen, likes);
            else snprintf(label, sizeof(label), "Seen by %d", seen);
        }
        status_viewer_dialog_render(&app->feed.viewer, area, items, count, app->thumbs, &app->media_sources,
                                    settings(app)->use_24h_clock, label[0] ? label : NULL);
        if (status_feed_dialogs_listing_viewers(&app->feed)) {
            status_viewers_dialog_render(&app->feed.viewers, area, viewers, seen, viewer_name, app, settings(app)->use_24h_clock);
        }
        status_update_array_free(items, count);
        return;
    }
    StatusAuthor authors[MAX_AUTHORS];
    int n = status_feed_manager_authors(app->deps.feed, app->feed.list.archived, authors, MAX_AUTHORS);
    status_list_dialog_render(&app->feed.list, area, authors, n, author_name, app, settings(app)->use_24h_clock);
}

int tui_app_statuses_unseen(TuiApp *app) {
    return app->deps.feed ? status_feed_manager_unviewed_authors(app->deps.feed) : 0;
}
