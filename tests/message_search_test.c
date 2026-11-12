/* Message search works with and without SQLite's FTS5 (Ubuntu's SQLCipher
 * has none), messages still save without it, and the index catches up once
 * FTS5 is there again. */
#include "core/settings.h"
#include "managers/messaging_manager.h"
#include "resource_access/sqlite_chat_store.h"
#include "resource_access/sqlite_contact_store.h"
#include "resource_access/sqlite_database.h"
#include "resource_access/sqlite_jid_alias_store.h"
#include "resource_access/sqlite_message_store.h"
#include "resource_access/sqlite_reaction_store.h"
#include "resource_access/sqlite_receipt_store.h"
#include "resource_access/text_chat_exporter.h"
#include "utilities/event_queue.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

static int failures = 0;

#define CHECK(cond, what) do { if (!(cond)) { fprintf(stderr, "FAIL: %s\n", what); failures++; } } while (0)

static void save(IMessageStore *s, const char *id, const char *text, int64_t ts) {
    Message m;
    message_init(&m);
    snprintf(m.id, sizeof(m.id), "%s", id);
    snprintf(m.chat_jid, sizeof(m.chat_jid), "27820000000@s.whatsapp.net");
    m.timestamp = ts;
    message_set_text(&m, text);
    CHECK(s->save(s, &m) == 0, "a message saves");
    message_dispose(&m);
}

static int found(IMessageStore *s, const char *query, const char *id) {
    Message *hits = NULL;
    int n = 0;
    s->search(s, query, 50, &hits, &n);
    int yes = 0;
    for (int i = 0; i < n; i++) if (strcmp(hits[i].id, id) == 0) yes = 1;
    message_array_free(hits, n);
    return yes;
}

static int hits_for(IMessageStore *s, const char *query) {
    Message *hits = NULL;
    int n = 0;
    s->search(s, query, 50, &hits, &n);
    message_array_free(hits, n);
    return n;
}

static int fake_ok(IMessageGateway *g) { (void)g; return 0; }
static void fake_notify(INotifier *n, const Notification *note) { (void)n; (void)note; }

static int manager_finds(MessagingManager *m, const char *query, int include_private, const char *id) {
    Message *hits = NULL;
    int n = 0;
    messaging_manager_search(m, query, 50, include_private, &hits, &n);
    int yes = 0;
    for (int i = 0; i < n; i++) if (strcmp(hits[i].id, id) == 0) yes = 1;
    message_array_free(hits, n);
    return yes;
}

/* tawk's own search leaves a private chat out; agents ask with include_private and see it. */
static void private_search(void) {
    char path[] = "/tmp/tawk-private-search-XXXXXX";
    int fd = mkstemp(path);
    if (fd < 0) { perror("mkstemp"); failures++; return; }
    close(fd);
    unlink(path);
    sqlite3 *db = sqlite_database_open(path, NULL);
    if (!db) { failures++; return; }
    Settings settings;
    settings_set_defaults(&settings);
    IMessageGateway gw;
    memset(&gw, 0, sizeof(gw));
    gw.start = fake_ok;
    gw.connect = fake_ok;
    INotifier notifier = { NULL, fake_notify, NULL };
    EventQueue *events = event_queue_create(16);
    IMessageStore *messages = sqlite_message_store_create(db, ACCOUNT_ID_FIRST);
    IChatStore *chats = sqlite_chat_store_create(db, ACCOUNT_ID_FIRST);
    IContactStore *contacts = sqlite_contact_store_create(db, ACCOUNT_ID_FIRST);
    IJidAliasStore *aliases = sqlite_jid_alias_store_create(db, ACCOUNT_ID_FIRST);
    IReactionStore *reactions = sqlite_reaction_store_create(db, ACCOUNT_ID_FIRST);
    IReceiptStore *receipts = sqlite_receipt_store_create(db, ACCOUNT_ID_FIRST);
    IChatExporter *exporter = text_chat_exporter_create();
    MessagingManagerDeps deps = { &gw, messages, chats, contacts, aliases, reactions, receipts, &notifier, events,
                                  &settings, NULL, exporter, NULL, NULL, ACCOUNT_ID_FIRST, NULL, NULL, NULL, NULL };
    MessagingManager *m = messaging_manager_create(&deps);
    Chat c;
    chat_init(&c, "27820000000@s.whatsapp.net");
    snprintf(c.name, sizeof(c.name), "Someone");
    chats->upsert(chats, &c);
    save(messages, "P", "the secret braai", 300);

    messaging_manager_set_private(m, "27820000000@s.whatsapp.net", 0);
    CHECK(manager_finds(m, "secret", 0, "P"), "a public chat's message is found by tawk's search");
    messaging_manager_set_private(m, "27820000000@s.whatsapp.net", 1);
    CHECK(!manager_finds(m, "secret", 0, "P"), "a private chat's message is not, with include_private off");
    CHECK(manager_finds(m, "secret", 1, "P"), "and is, with include_private on, as agents ask");

    messaging_manager_destroy(m);
    event_queue_close(events);
    event_queue_destroy(events);
    exporter->destroy(exporter);
    receipts->destroy(receipts);
    reactions->destroy(reactions);
    aliases->destroy(aliases);
    contacts->destroy(contacts);
    chats->destroy(chats);
    messages->destroy(messages);
    sqlite_database_close(db);
    char side[600];
    const char *suffixes[] = { "", "-wal", "-shm", NULL };
    for (int i = 0; suffixes[i]; i++) { snprintf(side, sizeof(side), "%s%s", path, suffixes[i]); unlink(side); }
}

int main(void) {
    private_search();
    char path[] = "/tmp/tawk-search-test-XXXXXX";
    int fd = mkstemp(path);
    if (fd < 0) { perror("mkstemp"); return 1; }
    close(fd);
    unlink(path);

    sqlite3 *db = sqlite_database_open(path, NULL);
    CHECK(db != NULL, "the database opens on this SQLite, with or without FTS5");
    if (!db) return 1;
    IMessageStore *s = sqlite_message_store_create(db, ACCOUNT_ID_FIRST);
    save(s, "A", "Hello world, see you at the braai", 100);
    save(s, "B", "Worldwide shipping is 50% off", 200);
    CHECK(found(s, "hello wor", "A") && !found(s, "hello wor", "B"), "every word must match");
    CHECK(found(s, "braai", "A"), "a word is found");

    /* As on a SQLite without FTS5: the index's triggers are gone. */
    sqlite3_exec(db, "DROP TRIGGER IF EXISTS messages_fts_ai; DROP TRIGGER IF EXISTS messages_fts_ad;"
                     "DROP TRIGGER IF EXISTS messages_fts_au;", NULL, NULL, NULL);
    save(s, "C", "Late message about the braai", 300);
    CHECK(found(s, "braai", "C") && found(s, "braai", "A"), "without the index, search still finds messages");
    CHECK(found(s, "50%", "B") && hits_for(s, "5_%") == 0, "% and _ are matched literally");
    CHECK(hits_for(s, "nothing-like-this") == 0, "no match finds nothing");
    s->destroy(s);
    sqlite_database_close(db);

    /* Opening again: with FTS5 the index returns and includes message C. */
    db = sqlite_database_open(path, NULL);
    CHECK(db != NULL, "the database opens again");
    if (db) {
        s = sqlite_message_store_create(db, ACCOUNT_ID_FIRST);
        CHECK(found(s, "late braai", "C"), "messages saved without the index are found after it returns");
        s->destroy(s);
        sqlite_database_close(db);
    }
    char side[600];
    const char *suffixes[] = { "", "-wal", "-shm", NULL };
    for (int i = 0; suffixes[i]; i++) { snprintf(side, sizeof(side), "%s%s", path, suffixes[i]); unlink(side); }
    if (failures) return 1;
    printf("ok: messages are searched with an FTS5 index, or without one where SQLite lacks it\n");
    return 0;
}
