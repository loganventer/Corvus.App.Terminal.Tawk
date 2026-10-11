#ifndef APP_ENGINES_TABLE_CATALOG_H
#define APP_ENGINES_TABLE_CATALOG_H

#include "core/table_rule.h"

/* Every table tawk keeps, and what moving an account does with it. A new
 * table has no rule until someone adds one here, and the tests fail until
 * they do, so a table can never be left out of a move by accident. */
const TableRule *table_catalog_rules(int *count);
/* The scope of `table`; TABLE_SCOPE_UNKNOWN when it has no rule. */
TableScope       table_catalog_scope(const char *table);
const TableRule *table_catalog_find(const char *table);
/* How many of `tables` have no rule; the first of them goes to `first` when it is not NULL. */
int              table_catalog_unclassified(const char (*tables)[64], int count, const char **first);

#endif
