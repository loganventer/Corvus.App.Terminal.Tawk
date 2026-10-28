/* A chat read on another device, the phone or another tawk: the event that says so, and the unread count
 * that follows it. */
#include "core/event.h"
#include "core/settings.h"
#include "managers/messaging_manager.h"
#include "resource_access/json_protocol.h"
#include "resource_access/sqlite_chat_store.h"
#include "resource_access/sqlite_contact_store.h"
#include "resource_access/sqlite_database.h"
#include "resource_access/sqlite_jid_alias_store.h"
#include "resource_access/sqlite_message_store.h"
#include "resource_access/sqlite_reaction_store.h"
#include "resource_access/sqlite_receipt_store.h"
#include "resource_access/text_chat_exporter.h"
#include "utilities/event_queue.h"
#include "utilities/str_util.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

static int failures = 0;

#define CHECK(cond, what) do { if (!(cond)) { fprintf(stderr, "FAIL: %s\n", what); failures++; } } while (0)

#define CHAT "27820000001@s.whatsapp.net"

static void test_decode(void) {
    Event e;
    event_init(&e, EVENT_NONE);
    CHECK(json_protocol_decode("{\"evt\":\"chat_read\",\"jid\":\"" CHAT "\",\"ts\":1790000060}", &e) == 0 &&
          e.type == EVENT_CHAT_READ && strcmp(e.chat.jid, CHAT) == 0 && e.at == 1790000060, "a chat read elsewhere is decoded");
    event_dispose(&e);
    event_init(&e, EVENT_NONE);
    CHECK(json_protocol_decode("{\"evt\":\"chat_read\",\"jid\":\"" CHAT "\"}", &e) != 0, "one without a time is refused");
    event_dispose(&e);
    event_init(&e, EVENT_NONE);
    CHECK(json_protocol_decode("{\"evt\":\"chat_read\",\"ts\":5}", &e) != 0, "one without a chat is refused");
    event_dispose(&e);
}

static int reads;

static int fake_start(IMessageGateway *self) { (void)self; return 0; }
static int fake_connect(IMessageGateway *self) { (void)self; return 0; }
static int fake_reconnect(IMessageGateway *self) { (void)self; return 0; }
static int fake_presence(IMessageGateway *self, int available) { (void)self; (void)available; return 0; }
static int fake_subscribe(IMessageGateway *self, const char *jid) { (void)self; (void)jid; return 0; }
static int fake_mark_read(IMessageGateway *self, const ReadRequest *r) { (void)self; (void)r; reads++; return 0; }
static void fake_notify(INotifier *self, const Notification *n) { (void)self; (void)n; }

static void store_message(IMessageStore *store, const char *id, int64_t ts, int from_me) {
    Message msg;
    message_init(&msg);
    str_copy(msg.id, sizeof(msg.id), id);
    str_copy(msg.chat_jid, sizeof(msg.chat_jid), CHAT);
    str_copy(msg.sender_jid, sizeof(msg.sender_jid), from_me ? "me@s.whatsapp.net" : CHAT);
    msg.type = MESSAGE_TYPE_TEXT;
    msg.from_me = from_me;
    msg.timestamp = ts;
    message_set_text(&msg, "hello");
    store->save(store, &msg);
    message_dispose(&msg);
}

static void push_read(EventQueue *q, int64_t at) {
    Event e;
    event_init(&e, EVENT_CHAT_READ);
    str_copy(e.chat.jid, sizeof(e.chat.jid), CHAT);
    e.at = at;
    event_queue_push(q, &e);
}

static int unread_of(IChatStore *chats) {
    Chat chat;
    return chats->get(chats, CHAT, &chat) == 0 ? chat.unread : -99;
}

static void test_unread_follows(void) {
    char path[] = "/tmp/tawk-read-elsewhere-XXXXXX";
    int fd = mkstemp(path);
    if (fd < 0) { perror("mkstemp"); failures++; return; }
    close(fd);
    unlink(path);
    sqlite3 *db = sqlite_database_open(path, NULL);
    CHECK(db != NULL, "the database opens");
    if (!db) return;

    Settings settings;
    settings_set_defaults(&settings);
    IMessageGateway gateway;
    memset(&gateway, 0, sizeof(gateway));
    gateway.start = fake_start;
    gateway.connect = fake_connect;
    gateway.reconnect = fake_reconnect;
    gateway.presence = fake_presence;
    gateway.subscribe = fake_subscribe;
    gateway.mark_read = fake_mark_read;
    INotifier notifier = { NULL, fake_notify, NULL };
    EventQueue *events = event_queue_create(64);
    IMessageStore *messages = sqlite_message_store_create(db, ACCOUNT_ID_FIRST);
    IChatStore *chats = sqlite_chat_store_create(db, ACCOUNT_ID_FIRST);
    IContactStore *contacts = sqlite_contact_store_create(db, ACCOUNT_ID_FIRST);
    IJidAliasStore *aliases = sqlite_jid_alias_store_create(db, ACCOUNT_ID_FIRST);
    IReactionStore *reactions = sqlite_reaction_store_create(db, ACCOUNT_ID_FIRST);
    IReceiptStore *receipts = sqlite_receipt_store_create(db, ACCOUNT_ID_FIRST);
    IChatExporter *exporter = text_chat_exporter_create();
    MessagingManagerDeps deps = { &gateway, messages, chats, contacts, aliases, reactions, receipts, &notifier, events,
                                  &settings, NULL, exporter, NULL, NULL, ACCOUNT_ID_FIRST, NULL, NULL, NULL, NULL };

    Chat chat;
    chat_init(&chat, CHAT);
    str_copy(chat.name, sizeof(chat.name), "Mom");
    chats->upsert(chats, &chat);
    store_message(messages, "M1", 1790000000, 0);
    store_message(messages, "M2", 1790000060, 0);
    store_message(messages, "M3", 1790000120, 0);
    store_message(messages, "M4", 1790000180, 1);     /* yours, after the three: it is never unread */
    store_message(messages, "M5", 1790000240, 0);
    chats->set_unread(chats, CHAT, 4);

    MessagingManager *m = messaging_manager_create(&deps);
    ManagerChanges ch;
    messaging_manager_start(m);

    push_read(events, 1790000060);
    messaging_manager_tick(m, &ch);
    CHECK(unread_of(chats) == 2, "read up to the second message leaves what came after it unread");

    push_read(events, 1790000060);
    messaging_manager_tick(m, &ch);
    CHECK(unread_of(chats) == 2, "the same mark again changes nothing");

    push_read(events, 1790000300);
    messaging_manager_tick(m, &ch);
    CHECK(unread_of(chats) == 0, "read up to the newest message clears it");

    chats->set_unread(chats, CHAT, 0);
    push_read(events, 1790000300);
    messaging_manager_tick(m, &ch);
    CHECK(unread_of(chats) == 0, "a chat with nothing unread stays that way");
    CHECK(reads == 0, "reading elsewhere sends no read receipts from here");

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
    unlink(path);
}

int main(void) {
    test_decode();
    test_unread_follows();
    if (failures) { fprintf(stderr, "%d failure(s)\n", failures); return 1; }
    puts("chat read elsewhere: ok");
    return 0;
}
