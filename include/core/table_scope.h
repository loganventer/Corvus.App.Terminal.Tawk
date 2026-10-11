#ifndef APP_CORE_TABLE_SCOPE_H
#define APP_CORE_TABLE_SCOPE_H

/* How a table of the database takes part in moving one account to another computer. */
typedef enum TableScope {
    TABLE_SCOPE_UNKNOWN = 0,    /* no rule yet: a new table that nobody decided about */
    TABLE_SCOPE_ACCOUNT,        /* has account_id; the account's rows travel */
    TABLE_SCOPE_BY_JID,         /* kept per chat for every account; the rows of the account's chats travel */
    TABLE_SCOPE_LEFT_BEHIND,    /* stays on this computer (counted, so you are told) */
    TABLE_SCOPE_GLOBAL          /* belongs to the installation, or is rebuilt from what travels */
} TableScope;

#endif
