#ifndef APP_CORE_MERGE_PLAN_H
#define APP_CORE_MERGE_PLAN_H

#include "core/account_id.h"
#include "core/media_path_map.h"

/* How the rows of a package are put into this database. */
typedef struct MergePlan {
    AccountId    source_account;    /* the id the rows carry in the package */
    AccountId    new_account;       /* the id they get here */
    MediaPathMap media;
} MergePlan;

#endif
