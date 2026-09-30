#ifndef APP_CLIENTS_TUI_PORTRAIT_SOURCE_H
#define APP_CLIENTS_TUI_PORTRAIT_SOURCE_H

/* Where the views get profile pictures from, without knowing who fetches
 * them. `picture` returns a file path, or NULL when there is none (yet). */
typedef struct PortraitSource {
    void       *ctx;
    const char *(*picture)(void *ctx, const char *jid);
} PortraitSource;

#endif
