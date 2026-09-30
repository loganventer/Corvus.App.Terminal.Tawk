#ifndef APP_CONTRACTS_I_CLIPBOARD_H
#define APP_CONTRACTS_I_CLIPBOARD_H

/* Puts text on the system clipboard. */
typedef struct IClipboard {
    void *ctx;
    int  (*copy)(struct IClipboard *self, const char *utf8);
    void (*destroy)(struct IClipboard *self);
} IClipboard;

#endif
