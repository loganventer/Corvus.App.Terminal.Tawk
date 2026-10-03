#ifndef APP_CLIENTS_TUI_SPLASH_VIEW_H
#define APP_CLIENTS_TUI_SPLASH_VIEW_H

#include <stdint.h>

#include "clients/tui/ui_rect.h"

/* The start-up screen, in the middle of the screen: the logo's symbol (a
 * speech bubble of bars) over the name in block letters. Both wipe in from the left with a bright
 * edge, a shine keeps sweeping across it, the tagline types itself out,
 * typing dots pulse under it, and the whole thing fades out at the end.
 * It is drawn while the backend connects and never delays it. */
typedef struct SplashView {
    int     active;
    int64_t started_ms;
    int64_t ends_ms;
} SplashView;

void splash_view_start(SplashView *view, int64_t now_ms);
/* Stops early (a key was pressed). */
void splash_view_skip(SplashView *view);
/* True while it should still be shown; turns itself off when its time is up. */
int  splash_view_active(SplashView *view, int64_t now_ms);
/* `detail` is a line under the logo (the version); `status` what tawk is doing. */
void splash_view_render(const SplashView *view, UiRect screen, int64_t now_ms, const char *detail, const char *status);

#endif
