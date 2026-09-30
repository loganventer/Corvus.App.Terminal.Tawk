#ifndef APP_CORE_BACKUP_PATHS_H
#define APP_CORE_BACKUP_PATHS_H

/* Where tawk keeps what a backup holds, as the settings resolve them. */
typedef struct BackupPaths {
    const char *data_dir;       /* the staging folder is made here, next to the data */
    const char *db_path;
    const char *config_path;
    const char *themes_dir;     /* your own themes */
    const char *media_dir;
    const char *auth_dir;       /* the WhatsApp login */
} BackupPaths;

#endif
