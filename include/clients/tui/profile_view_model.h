#ifndef APP_CLIENTS_TUI_PROFILE_VIEW_MODEL_H
#define APP_CLIENTS_TUI_PROFILE_VIEW_MODEL_H

/* What the profile view shows about your own account. */
typedef struct ProfileViewModel {
    const char *jid;
    const char *name;
    const char *about;          /* "" while unknown */
    const char *picture;        /* preview path, or NULL */
    int         busy[3];        /* a change to the name, about or photo is being saved */
} ProfileViewModel;

#endif
