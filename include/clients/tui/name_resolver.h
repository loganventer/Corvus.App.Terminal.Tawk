#ifndef APP_CLIENTS_TUI_NAME_RESOLVER_H
#define APP_CLIENTS_TUI_NAME_RESOLVER_H

#include <stddef.h>

/* Turns a JID into a display name ("You" for yourself). Injected into views
 * so they never reach into managers. */
typedef struct NameResolver {
    void *ctx;
    void (*resolve)(void *ctx, const char *jid, char *out, size_t size);
} NameResolver;

#endif
