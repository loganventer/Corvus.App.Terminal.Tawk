#include "managers/settings_manager.h"
#include "utilities/log.h"

#include <stdlib.h>

struct SettingsManager {
    ISettingsStore   *store;
    IThemeRepository *themes;
    Settings          current;
    unsigned          revision;
};

SettingsManager *settings_manager_create(ISettingsStore *store, IThemeRepository *themes) {
    SettingsManager *m = calloc(1, sizeof(*m));
    if (!m) return NULL;
    m->store = store;
    m->themes = themes;
    settings_set_defaults(&m->current);
    return m;
}

void settings_manager_destroy(SettingsManager *m) { free(m); }

int settings_manager_load(SettingsManager *m) {
    return m->store->load(m->store, &m->current);
}

const Settings *settings_manager_current(SettingsManager *m) { return &m->current; }

int settings_manager_apply(SettingsManager *m, const Settings *updated) {
    int trusted = m->current.config_trusted;
    m->current = *updated;
    m->revision++;
    m->current.config_trusted = trusted;
    if (m->store->save(m->store, &m->current) != 0) {
        LOG_ERROR("settings could not be saved");
        return -1;
    }
    /* The store just wrote a fresh 0600 file owned by us, and command values
     * from an untrusted file were never loaded, so the file is now trusted. */
    m->current.config_trusted = 1;
    return 0;
}

IThemeRepository *settings_manager_themes(SettingsManager *m) { return m->themes; }
unsigned          settings_manager_revision(SettingsManager *m) { return m->revision; }

const Theme *settings_manager_theme(SettingsManager *m) {
    /* An unknown theme (a removed file) falls back to the default, not to the first one alphabetically. */
    int index = m->themes->index_of(m->themes, m->current.theme);
    if (index < 0) index = m->themes->index_of(m->themes, SETTINGS_DEFAULT_THEME);
    return m->themes->at(m->themes, index < 0 ? 0 : index);
}
