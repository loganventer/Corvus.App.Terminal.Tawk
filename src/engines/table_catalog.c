#include "engines/table_catalog.h"

#include <string.h>

static const TableRule RULES[] = {
    { "messages",           TABLE_SCOPE_ACCOUNT,     NULL },
    { "chats",              TABLE_SCOPE_ACCOUNT,     NULL },
    { "contacts",           TABLE_SCOPE_ACCOUNT,     NULL },
    { "jid_aliases",        TABLE_SCOPE_ACCOUNT,     NULL },
    { "reactions",          TABLE_SCOPE_ACCOUNT,     NULL },
    { "message_receipts",   TABLE_SCOPE_ACCOUNT,     NULL },
    { "profiles",           TABLE_SCOPE_ACCOUNT,     NULL },
    { "statuses",           TABLE_SCOPE_ACCOUNT,     NULL },
    { "transcripts",        TABLE_SCOPE_ACCOUNT,     NULL },
    { "summaries",          TABLE_SCOPE_ACCOUNT,     NULL },
    { "summary_declines",   TABLE_SCOPE_ACCOUNT,     NULL },
    { "sent_ids",           TABLE_SCOPE_ACCOUNT,     NULL },
    { "chat_prefs",         TABLE_SCOPE_BY_JID,      "jid" },
    { "chat_labels",        TABLE_SCOPE_BY_JID,      "jid" },
    { "chat_reminders",     TABLE_SCOPE_BY_JID,      "jid" },
    { "scheduled_messages", TABLE_SCOPE_LEFT_BEHIND, NULL },
    { "automation_log",     TABLE_SCOPE_LEFT_BEHIND, NULL },
    { "accounts",           TABLE_SCOPE_GLOBAL,      NULL },
};

const TableRule *table_catalog_rules(int *count) {
    if (count) *count = (int)(sizeof(RULES) / sizeof(RULES[0]));
    return RULES;
}

const TableRule *table_catalog_find(const char *table) {
    if (!table) return NULL;
    for (size_t i = 0; i < sizeof(RULES) / sizeof(RULES[0]); i++) {
        if (strcmp(RULES[i].table, table) == 0) return &RULES[i];
    }
    return NULL;
}

/* SQLite's own bookkeeping, and the search index, which is rebuilt as the messages arrive. */
static int is_machinery(const char *table) {
    return strncmp(table, "sqlite_", 7) == 0 || strncmp(table, "messages_fts", 12) == 0;
}

TableScope table_catalog_scope(const char *table) {
    const TableRule *rule = table_catalog_find(table);
    if (rule) return rule->scope;
    return table && is_machinery(table) ? TABLE_SCOPE_GLOBAL : TABLE_SCOPE_UNKNOWN;
}

int table_catalog_unclassified(const char (*tables)[64], int count, const char **first) {
    int unknown = 0;
    if (first) *first = NULL;
    for (int i = 0; i < count; i++) {
        if (table_catalog_scope(tables[i]) != TABLE_SCOPE_UNKNOWN) continue;
        if (!unknown && first) *first = tables[i];
        unknown++;
    }
    return unknown;
}
