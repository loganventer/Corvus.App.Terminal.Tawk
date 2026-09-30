#ifndef APP_CLIENTS_TUI_INCOMING_CALL_VIEW_H
#define APP_CLIENTS_TUI_INCOMING_CALL_VIEW_H

#include <stdint.h>

#include "clients/tui/image_placement.h"
#include "clients/tui/thumbnail_cache.h"
#include "clients/tui/ui_rect.h"

typedef enum IncomingCallChoice {
    INCOMING_CALL_NONE = 0,
    INCOMING_CALL_DECLINE,
    INCOMING_CALL_DISMISS        /* answer on the phone */
} IncomingCallChoice;

/* The ringing call: the caller's portrait and name in a pulsing box, with
 * Decline and "Answer on phone" (which only closes the prompt). */
typedef struct IncomingCallView {
    UiRect last_rect;
    UiRect decline_button;
    UiRect dismiss_button;
} IncomingCallView;

IncomingCallChoice incoming_call_key(int is_key_code, int ch);
IncomingCallChoice incoming_call_click(const IncomingCallView *view, int y, int x);
/* Returns 1 when *placement holds the portrait as a pixel image. */
int                incoming_call_render(IncomingCallView *view, UiRect area, const char *jid, const char *name,
                                        const char *picture, ThumbnailCache *thumbs, int pixel_images,
                                        int64_t ringing_ms, ImagePlacement *placement);

#endif
