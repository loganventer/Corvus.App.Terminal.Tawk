#ifndef APP_CORE_TABLE_RULE_H
#define APP_CORE_TABLE_RULE_H

#include "core/table_scope.h"

/* What moving an account does with one table. */
typedef struct TableRule {
    const char *table;
    TableScope  scope;
    const char *key_column;     /* the chat column of a BY_JID table; NULL otherwise */
} TableRule;

#endif
