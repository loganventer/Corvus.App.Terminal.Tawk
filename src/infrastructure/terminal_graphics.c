#include "infrastructure/terminal_graphics.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <unistd.h>

static int env_is(const char *name, const char *value) {
    const char *v = getenv(name);
    return v && strcmp(v, value) == 0;
}

static int env_set(const char *name) {
    const char *v = getenv(name);
    return v && *v;
}

int terminal_graphics_sixel(void) {
    if (env_set("TMUX") || env_set("STY")) return 0;
    if (env_set("WT_SESSION")) return 1;                   /* Windows Terminal 1.22+ */
    if (env_is("TERM_PROGRAM", "WezTerm") || env_is("TERM_PROGRAM", "contour")) return 1;
    if (env_is("TERM_PROGRAM", "iTerm.app") || env_is("LC_TERMINAL", "iTerm2")) {   /* Sixel since 3.5 */
        const char *v = getenv(env_is("TERM_PROGRAM", "iTerm.app") ? "TERM_PROGRAM_VERSION" : "LC_TERMINAL_VERSION");
        int major = 0, minor = 0;
        if (v && sscanf(v, "%d.%d", &major, &minor) >= 1 && (major > 3 || (major == 3 && minor >= 5))) return 1;
    }
    const char *term = getenv("TERM");
    if (!term) return 0;
    return strncmp(term, "foot", 4) == 0 || strncmp(term, "mlterm", 6) == 0 || strstr(term, "sixel") != NULL;
}

void terminal_graphics_cell_pixels(int *width, int *height) {
    *width = 10;
    *height = 20;
    if (env_set("WT_SESSION")) return;                     /* scales Sixel to a 10 x 20 cell */
    struct winsize ws;
    if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) == 0 && ws.ws_xpixel && ws.ws_ypixel && ws.ws_col && ws.ws_row) {
        *width = ws.ws_xpixel / ws.ws_col;
        *height = ws.ws_ypixel / ws.ws_row;
        if (*width < 4 || *height < 8) { *width = 10; *height = 20; }
    }
}
