#ifndef APP_CONTRACTS_I_PROFILE_EDITOR_H
#define APP_CONTRACTS_I_PROFILE_EDITOR_H

/* Changes your own WhatsApp profile. Each call only sends the request; the
 * outcome arrives later as an EVENT_PROFILE_UPDATED. Owned by the gateway
 * that hands it out, so it has no destroy of its own. */
typedef struct IProfileEditor {
    void *ctx;
    int  (*set_name)(struct IProfileEditor *self, const char *name);
    int  (*set_about)(struct IProfileEditor *self, const char *text);
    /* `path` is a JPEG or PNG inside the media folder; the backend crops it square. */
    int  (*set_picture)(struct IProfileEditor *self, const char *path);
    int  (*remove_picture)(struct IProfileEditor *self);
} IProfileEditor;

#endif
