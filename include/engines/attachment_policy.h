#ifndef APP_ENGINES_ATTACHMENT_POLICY_H
#define APP_ENGINES_ATTACHMENT_POLICY_H

#include "core/attachment_rules.h"
#include "core/attachment_verdict.h"
#include "utilities/file_facts.h"

/* Whether a file may be sent as an attachment. `resolved_path` is the file's
 * absolute path with links resolved, so a link cannot point past a rule.
 * The file must exist, be a plain, non-empty file within `rules->max_bytes`,
 * and not lie in a protected path nor in a place under the home folder that
 * holds keys and credentials (.ssh, .gnupg, .aws and the like), nor be named
 * as a key or credentials file. */
AttachmentVerdict attachment_policy_check(const char *resolved_path, const FileFacts *facts, const AttachmentRules *rules);

#endif
