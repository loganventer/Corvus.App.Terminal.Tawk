#ifndef APP_CORE_ATTACHMENT_RULES_H
#define APP_CORE_ATTACHMENT_RULES_H

/* What an agent may not send, and how big a file may be. All paths are
 * absolute and already resolved (no links, no "..") by whoever fills this. */
typedef struct AttachmentRules {
    const char *const *protected_paths;   /* folders or files that are never sent; a folder covers all inside it */
    int                protected_count;
    const char        *home;              /* the user's home folder, "" when unknown */
    long long          max_bytes;
} AttachmentRules;

#endif
