#ifndef APP_CONTRACTS_I_SOUND_CATALOG_H
#define APP_CONTRACTS_I_SOUND_CATALOG_H

#include "core/sound_option.h"

/* The notification sounds there are to choose from, sorted by id. */
typedef struct ISoundCatalog {
    void *ctx;
    int                (*count)(struct ISoundCatalog *self);
    const SoundOption *(*at)(struct ISoundCatalog *self, int index);
    /* Index of the sound stored at this path, or -1 (a file of your own that is
     * not in the sound folders is not in the catalog). */
    int                (*index_of_path)(struct ISoundCatalog *self, const char *path);
    /* Index of the sound with this id, or -1. */
    int                (*index_of_id)(struct ISoundCatalog *self, const char *id);
    /* Reads the folders again (picks up newly added files). */
    int                (*reload)(struct ISoundCatalog *self);
    void               (*destroy)(struct ISoundCatalog *self);
} ISoundCatalog;

#endif
