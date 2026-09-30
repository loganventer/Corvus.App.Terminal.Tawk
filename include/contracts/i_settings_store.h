#ifndef APP_CONTRACTS_I_SETTINGS_STORE_H
#define APP_CONTRACTS_I_SETTINGS_STORE_H

#include "core/settings.h"

typedef struct ISettingsStore {
    void *ctx;
    /* Overlays persisted values on top of the defaults already in `settings`.
     * Writes a complete file when none exists. */
    int  (*load)(struct ISettingsStore *self, Settings *settings);
    int  (*save)(struct ISettingsStore *self, const Settings *settings);
    void (*destroy)(struct ISettingsStore *self);
} ISettingsStore;

#endif
