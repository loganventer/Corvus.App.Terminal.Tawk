#ifndef APP_ENGINES_ACCOUNT_IDENTITY_RULE_H
#define APP_ENGINES_ACCOUNT_IDENTITY_RULE_H

#include "core/identity_verdict.h"

/* Whether `linked_jid` is the number a moved account was expecting. The
 * device part of a JID ("27821234567:12@s.whatsapp.net") does not count: it
 * is the same person on another device. */
IdentityVerdict account_identity_rule(const char *expected_jid, const char *linked_jid);

#endif
