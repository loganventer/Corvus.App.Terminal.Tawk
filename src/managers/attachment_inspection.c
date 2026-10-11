#include "managers/attachment_inspection.h"

#include "core/message_type.h"
#include "engines/attachment_policy.h"
#include "engines/media_type_detector.h"
#include "utilities/file_inspector.h"
#include "utilities/path_util.h"
#include "utilities/str_util.h"

#include <stdio.h>
#include <string.h>

#define PROTECTED_MAX 6

/* Adds `path` to the list, resolved when it exists (a link into it must not slip past). */
static void protect(char (*store)[1024], const char **list, int *count, const char *path) {
    if (!path || !*path || *count >= PROTECTED_MAX) return;
    if (file_inspector_resolve(path, store[*count], sizeof(store[*count])) != 0) str_copy(store[*count], sizeof(store[*count]), path);
    list[*count] = store[*count];
    (*count)++;
}

AttachmentVerdict attachment_inspection_run(const Settings *settings, const char *path, AttachmentInfo *info) {
    if (info) memset(info, 0, sizeof(*info));
    char real[1024];
    if (!path || file_inspector_resolve(path, real, sizeof(real)) != 0) return ATTACHMENT_NOT_FOUND;

    FileFacts facts;
    file_inspector_read(real, &facts);

    char store[PROTECTED_MAX][1024];
    const char *protected_paths[PROTECTED_MAX];
    int count = 0;
    char own[600];
    settings_auth_dir(settings, own, sizeof(own));
    protect(store, protected_paths, &count, own);
    path_join(own, sizeof(own), settings->data_dir, "accounts");
    protect(store, protected_paths, &count, own);
    settings_db_path(settings, own, sizeof(own));
    protect(store, protected_paths, &count, own);
    char wal[640];
    snprintf(wal, sizeof(wal), "%s-wal", own);
    protect(store, protected_paths, &count, wal);
    snprintf(wal, sizeof(wal), "%s-shm", own);
    protect(store, protected_paths, &count, wal);
    protect(store, protected_paths, &count, settings->config_path);

    char home[512], home_real[1024];
    path_expand_home("~", home, sizeof(home));
    if (file_inspector_resolve(home, home_real, sizeof(home_real)) != 0) str_copy(home_real, sizeof(home_real), home);

    AttachmentRules rules = { protected_paths, count, home_real, ATTACHMENT_MAX_BYTES };
    AttachmentVerdict verdict = attachment_policy_check(real, &facts, &rules);

    if (info && facts.exists) {
        const char *slash = strrchr(real, '/');
        str_copy(info->name, sizeof(info->name), slash ? slash + 1 : real);
        info->kind = message_type_name(media_type_detect(real));
        info->size_bytes = facts.size_bytes;
    }
    return verdict;
}
