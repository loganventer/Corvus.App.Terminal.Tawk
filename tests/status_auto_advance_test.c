/* Statuses step to the next by themselves after a while, as on the phone,
 * and wait while there is a reason to. */
#include "clients/tui/status_feed_dialogs.h"
#include "clients/tui/status_viewer_dialog.h"

#include <stdio.h>
#include <string.h>

static int failures = 0;

#define CHECK(cond, what) do { if (!(cond)) { fprintf(stderr, "FAIL: %s\n", what); failures++; } } while (0)

#define SHOW 6000

/* Runs the timer in 100 ms steps, as the app's loop does; returns the last result that was not "nothing". */
static PopupResult run(StatusViewerDialog *d, int64_t *now, int ms, int hold) {
    PopupResult last = POPUP_NONE;
    for (int t = 0; t < ms; t += 100) {
        *now += 100;
        PopupResult r = status_viewer_dialog_tick(d, *now, SHOW, hold);
        if (r != POPUP_NONE) last = r;
    }
    return last;
}

int main(void) {
    StatusViewerDialog d;
    int64_t now = 1000000;
    status_viewer_dialog_open(&d, "27820000000@s.whatsapp.net", "Mom", 0, 3);
    status_viewer_dialog_tick(&d, now, SHOW, 0);                       /* the first tick only starts the clock */

    CHECK(run(&d, &now, SHOW - 500, 0) == POPUP_NONE && d.index == 0, "a status stays for its time");
    CHECK(d.elapsed_ms > 0 && d.elapsed_ms < SHOW && d.show_ms == SHOW, "the progress bar has its time to draw");
    CHECK(run(&d, &now, 600, 0) == POPUP_CHANGED && d.index == 1 && d.moved, "then the next one comes by itself");
    CHECK(d.elapsed_ms < 500, "the next status starts its own time");

    CHECK(run(&d, &now, SHOW * 2, 1) == POPUP_NONE && d.index == 1, "it waits while held (a download, the full size picture)");
    d.replying = 1;
    CHECK(run(&d, &now, SHOW * 2, 0) == POPUP_NONE && d.index == 1, "it waits while a reply is typed");
    d.replying = 0;

    run(&d, &now, SHOW - 1000, 0);
    status_viewer_dialog_key(&d, 0, 'p');                              /* back one by hand */
    CHECK(d.index == 0 && d.elapsed_ms == 0, "stepping by hand restarts the time");
    run(&d, &now, SHOW + 100, 0);
    run(&d, &now, SHOW + 100, 0);
    CHECK(d.index == 2 && d.open, "it plays through to the last one");

    now += 60000;                                                      /* the app stalled for a minute */
    CHECK(status_viewer_dialog_tick(&d, now, SHOW, 0) == POPUP_NONE && d.index == 2, "a stall does not skip a status");
    CHECK(run(&d, &now, SHOW + 100, 0) == POPUP_CLOSED && !d.open, "after the last it goes back to the list");
    CHECK(status_viewer_dialog_tick(&d, now + 100, SHOW, 0) == POPUP_NONE, "a closed viewer does nothing");

    /* The arrows beside a status step back and forth, even where they touch the picture. */
    status_viewer_dialog_open(&d, "27820000000@s.whatsapp.net", "Mom", 1, 3);
    d.last_rect = (UiRect){ 0, 0, 30, 76 };
    d.media_rect = (UiRect){ 4, 0, 20, 76 };
    d.prev_arrow = (UiRect){ 13, 0, 3, 2 };
    d.next_arrow = (UiRect){ 13, 74, 3, 2 };
    CHECK(status_viewer_dialog_click(&d, 14, 75) == POPUP_CHANGED && d.index == 2, "the right arrow goes forwards");
    CHECK(status_viewer_dialog_click(&d, 14, 1) == POPUP_CHANGED && d.index == 1, "the left arrow goes back");
    CHECK(status_viewer_dialog_click(&d, 14, 30) == POPUP_CHOSEN && d.index == 1, "a click on the picture still opens it");
    d.index = 0;
    d.prev_arrow = (UiRect){ 0, 0, 0, 0 };                           /* as drawn on the first status */
    d.media_rect = (UiRect){ 4, 10, 20, 50 };
    d.prev_zone = (UiRect){ 0, 0, 0, 0 };
    CHECK(status_viewer_dialog_click(&d, 14, 1) == POPUP_NONE && d.index == 0, "there is no way back from the first");

    /* Through the feed dialogs: the viewers list over a status holds it too. */
    StatusFeedDialogs f;
    memset(&f, 0, sizeof(f));
    status_feed_dialogs_open(&f);
    CHECK(status_feed_dialogs_tick(&f, now, SHOW, 0) == STATUS_FEED_NONE, "nothing advances in the list");
    status_feed_dialogs_view(&f, "27820000000@s.whatsapp.net", "My status", 0, 2);
    status_feed_dialogs_tick(&f, now, SHOW, 0);
    status_feed_dialogs_show_viewers(&f);
    StatusFeedRequest held = STATUS_FEED_NONE;
    for (int t = 0; t < SHOW * 2; t += 100) { now += 100; StatusFeedRequest r = status_feed_dialogs_tick(&f, now, SHOW, 0); if (r != STATUS_FEED_NONE) held = r; }
    CHECK(held == STATUS_FEED_NONE && status_feed_dialogs_index(&f) == 0, "it waits while the viewers list is open");

    if (failures) return 1;
    printf("ok: statuses advance by themselves and wait when they should\n");
    return 0;
}
