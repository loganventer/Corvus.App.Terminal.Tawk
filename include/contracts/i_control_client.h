#ifndef APP_CONTRACTS_I_CONTROL_CLIENT_H
#define APP_CONTRACTS_I_CONTROL_CLIENT_H

/* The calling end of the control socket, for shell commands. */
typedef struct IControlClient {
    void *ctx;
    /* Returns 0, or -1 when no tawk is listening. */
    int  (*connect)(struct IControlClient *self, const char *path);
    int  (*send)(struct IControlClient *self, const char *line);
    /* Waits up to `timeout_ms` (-1 for ever) for one line; returns it
     * (the caller frees it), or NULL when the time ran out or tawk went away. */
    char *(*read_line)(struct IControlClient *self, int timeout_ms);
    void (*destroy)(struct IControlClient *self);
} IControlClient;

#endif
