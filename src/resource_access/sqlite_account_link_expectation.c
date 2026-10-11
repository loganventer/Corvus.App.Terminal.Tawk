#include "resource_access/sqlite_account_link_expectation.h"
#include "utilities/str_util.h"

#include <stdlib.h>

static sqlite3 *db_of(IAccountLinkExpectation *self) { return (sqlite3 *)self->ctx; }

static int set_value(IAccountLinkExpectation *self, AccountId id, const char *jid) {
    sqlite3_stmt *st = NULL;
    if (sqlite3_prepare_v2(db_of(self), "UPDATE accounts SET expected_jid = ? WHERE id = ?", -1, &st, NULL) != SQLITE_OK) return -1;
    sqlite3_bind_text(st, 1, jid ? jid : "", -1, SQLITE_TRANSIENT);
    sqlite3_bind_int(st, 2, id);
    int rc = sqlite3_step(st);
    sqlite3_finalize(st);
    return rc == SQLITE_DONE ? 0 : -1;
}

static int expectation_expect(IAccountLinkExpectation *self, AccountId id, const char *jid) { return set_value(self, id, jid); }
static int expectation_clear(IAccountLinkExpectation *self, AccountId id) { return set_value(self, id, ""); }

static int expectation_expected(IAccountLinkExpectation *self, AccountId id, char *out, size_t size) {
    sqlite3_stmt *st = NULL;
    if (out && size) out[0] = '\0';
    if (sqlite3_prepare_v2(db_of(self), "SELECT expected_jid FROM accounts WHERE id = ?", -1, &st, NULL) != SQLITE_OK) return -1;
    sqlite3_bind_int(st, 1, id);
    int found = sqlite3_step(st) == SQLITE_ROW;
    if (found && out && size) str_copy(out, size, (const char *)sqlite3_column_text(st, 0));
    sqlite3_finalize(st);
    return found ? 0 : -1;
}

static void expectation_destroy(IAccountLinkExpectation *self) { free(self); }

IAccountLinkExpectation *sqlite_account_link_expectation_create(sqlite3 *db) {
    IAccountLinkExpectation *e = calloc(1, sizeof(*e));
    if (!e) return NULL;
    e->ctx = db;
    e->expect = expectation_expect;
    e->expected = expectation_expected;
    e->clear = expectation_clear;
    e->destroy = expectation_destroy;
    return e;
}
