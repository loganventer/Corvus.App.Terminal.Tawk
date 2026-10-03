#ifndef APP_CONTRACTS_I_ADMIN_TOKEN_STORE_H
#define APP_CONTRACTS_I_ADMIN_TOKEN_STORE_H

/* Where the admin token is put for the one program you hand it to: it
 * lets that program answer its own waiting requests while access is admin. */
typedef struct IAdminTokenStore {
    void *ctx;
    /* Replaces whatever was there; readable by you alone. */
    int  (*save)(struct IAdminTokenStore *self, const char *token);
    void (*remove)(struct IAdminTokenStore *self);
    void (*destroy)(struct IAdminTokenStore *self);
} IAdminTokenStore;

#endif
