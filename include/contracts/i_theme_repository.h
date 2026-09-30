#ifndef APP_CONTRACTS_I_THEME_REPOSITORY_H
#define APP_CONTRACTS_I_THEME_REPOSITORY_H

#include "core/theme.h"

typedef struct IThemeRepository {
    void *ctx;
    int          (*count)(struct IThemeRepository *self);
    const Theme *(*at)(struct IThemeRepository *self, int index);
    /* Index of the theme with this id, or -1. */
    int          (*index_of)(struct IThemeRepository *self, const char *id);
    /* Re-reads the theme folders (picks up newly added files). */
    int          (*reload)(struct IThemeRepository *self);
    void         (*destroy)(struct IThemeRepository *self);
} IThemeRepository;

#endif
