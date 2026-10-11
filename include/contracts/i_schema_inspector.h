#ifndef APP_CONTRACTS_I_SCHEMA_INSPECTOR_H
#define APP_CONTRACTS_I_SCHEMA_INSPECTOR_H

#include <stddef.h>

#define SCHEMA_TABLE_NAME_SIZE 64

/* What a database holds, without reading any of its rows. */
typedef struct ISchemaInspector {
    void *ctx;
    /* The names of its tables, up to `max`; returns how many, or -1. `db_file` names a plain database
     * file, or is NULL for the one tawk has open. */
    int  (*tables)(struct ISchemaInspector *self, const char *db_file, char (*out)[SCHEMA_TABLE_NAME_SIZE], int max);
    /* Its schema version, or -1. */
    int  (*version)(struct ISchemaInspector *self, const char *db_file);
    void (*destroy)(struct ISchemaInspector *self);
} ISchemaInspector;

#endif
