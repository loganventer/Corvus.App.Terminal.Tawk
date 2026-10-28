#ifndef APP_CONTRACTS_I_URL_OPENER_H
#define APP_CONTRACTS_I_URL_OPENER_H

typedef struct IUrlOpener {
    void *ctx;
    /* Opens an http:// or https:// address in the user's browser without blocking.
     * Returns 0 when a browser was started, -1 otherwise (any other address included). */
    int  (*open)(struct IUrlOpener *self, const char *url);
    void (*destroy)(struct IUrlOpener *self);
} IUrlOpener;

#endif
