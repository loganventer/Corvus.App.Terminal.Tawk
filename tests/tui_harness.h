/* A terminal client with two accounts' chats in a real database, a
 * settings file, and the private chat fakes, for tests that drive the
 * client's own code without a screen. Header-only: include it once. */
#ifndef TESTS_TUI_HARNESS_H
#define TESTS_TUI_HARNESS_H

#include "../src/clients/tui/tui_app_state.h"
#include "clients/i_account_directory.h"
#include "clients/tui/tui_app.h"
#include "managers/messaging_manager.h"
#include "managers/settings_manager.h"
#include "private_fakes.h"
#include "resource_access/ini_settings_store.h"
#include "resource_access/json_theme_repository.h"
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

#include <signal.h>
#include <stdlib.h>
#include <unistd.h>

#define HARNESS_ACCOUNTS 2

typedef struct HarnessAccount {
    IMessageStore    *messages;
    IChatStore       *chats;
    IContactStore    *contacts;
    IJidAliasStore   *aliases;
    IReactionStore   *reactions;
    IReceiptStore    *receipts;
    EventQueue       *events;
    MessagingManager *messaging;
    AccountServices   services;
} HarnessAccount;

typedef struct Harness {
    char              dir[64];
    sqlite3          *db;
    SettingsManager  *settings;
    IChatExporter    *exporter;
    IMessageGateway   gateway;
    INotifier         notifier;
    HarnessAccount    accounts[HARNESS_ACCOUNTS];
    IAccountDirectory directory;
    FakePrivacy       privacy;
    BlinkState        blink;
    volatile sig_atomic_t quit;
    TuiApp           *app;
} Harness;

static int  harness_ok(IMessageGateway *g) { (void)g; return 0; }
static void harness_notify(INotifier *n, const Notification *note) { (void)n; (void)note; }

static int harness_count(IAccountDirectory *d) { (void)d; return HARNESS_ACCOUNTS; }
static const AccountServices *harness_at(IAccountDirectory *d, int i) {
    Harness *h = (Harness *)d->ctx;
    return i >= 0 && i < HARNESS_ACCOUNTS ? &h->accounts[i].services : NULL;
}
static const AccountServices *harness_find(IAccountDirectory *d, AccountId id) {
    Harness *h = (Harness *)d->ctx;
    for (int i = 0; i < HARNESS_ACCOUNTS; i++) if (h->accounts[i].services.id == id) return &h->accounts[i].services;
    return NULL;
}

static void harness_account(Harness *h, int i) {
    HarnessAccount *a = &h->accounts[i];
    AccountId id = (AccountId)(i + 1);
    a->messages = sqlite_message_store_create(h->db, id);
    a->chats = sqlite_chat_store_create(h->db, id);
    a->contacts = sqlite_contact_store_create(h->db, id);
    a->aliases = sqlite_jid_alias_store_create(h->db, id);
    a->reactions = sqlite_reaction_store_create(h->db, id);
    a->receipts = sqlite_receipt_store_create(h->db, id);
    a->events = event_queue_create(16);
    MessagingManagerDeps deps = { &h->gateway, a->messages, a->chats, a->contacts, a->aliases, a->reactions, a->receipts,
                                  &h->notifier, a->events, settings_manager_current(h->settings), NULL, h->exporter,
                                  NULL, NULL, id, NULL, NULL, NULL, NULL };
    a->messaging = messaging_manager_create(&deps);
    a->services.id = id;
    a->services.label = i == 0 ? "First" : "Second";
    a->services.backend_name = "test";
    a->services.messaging = a->messaging;
}

/* A chat on account `index` (0 or 1), private or not. */
static void harness_chat(Harness *h, int index, const char *jid, const char *name, int is_private) {
    HarnessAccount *a = &h->accounts[index];
    Chat c;
    chat_init(&c, jid);
    str_copy(c.name, sizeof(c.name), name);
    c.last_ts = 1790000000;
    a->chats->upsert(a->chats, &c);
    messaging_manager_set_private(a->messaging, jid, is_private);
}

static int harness_open(Harness *h, int touch_id, const char *password) {
    memset(h, 0, sizeof(*h));
    str_copy(h->dir, sizeof(h->dir), "/tmp/tawk-tui-harness-XXXXXX");
    if (!mkdtemp(h->dir)) return -1;
    char db_path[128], config[128];
    snprintf(db_path, sizeof(db_path), "%s/tawk.db", h->dir);
    snprintf(config, sizeof(config), "%s/config.ini", h->dir);
    h->db = sqlite_database_open(db_path, NULL);
    if (!h->db) return -1;
    h->settings = settings_manager_create(ini_settings_store_create(config), json_theme_repository_create("themes", h->dir));
    settings_manager_load(h->settings);
    Settings s = *settings_manager_current(h->settings);
    s.reopen_last_chat = 0;
    settings_manager_apply(h->settings, &s);
    h->exporter = text_chat_exporter_create();
    h->gateway.start = harness_ok;
    h->gateway.connect = harness_ok;
    h->notifier = (INotifier){ NULL, harness_notify, NULL };
    for (int i = 0; i < HARNESS_ACCOUNTS; i++) harness_account(h, i);
    h->directory.ctx = h;
    h->directory.count = harness_count;
    h->directory.at = harness_at;
    h->directory.find = harness_find;
    fake_privacy_init(&h->privacy, touch_id, password);
    TuiAppDeps deps;
    memset(&deps, 0, sizeof(deps));
    deps.messaging = h->accounts[0].messaging;
    deps.settings = h->settings;
    deps.blink = &h->blink;
    deps.quit_requested = &h->quit;
    deps.directory = &h->directory;
    deps.active_account = 1;
    deps.privacy = h->privacy.manager;
    h->app = tui_app_create(&deps);
    return h->app ? 0 : -1;
}

static const char *harness_open_jid(Harness *h) { return messaging_manager_open_jid(h->app->deps.messaging); }

/* Types `text` and Enter into whatever private chat dialog is up. */
static void harness_type(Harness *h, const char *text) {
    for (const char *p = text; *p; p++) tui_app_private_key(h->app, 0, (unsigned char)*p);
    tui_app_private_key(h->app, 0, '\r');
}

static void harness_close(Harness *h) {
    if (h->app) tui_app_destroy(h->app);
    fake_privacy_dispose(&h->privacy);
    for (int i = 0; i < HARNESS_ACCOUNTS; i++) {
        HarnessAccount *a = &h->accounts[i];
        if (a->messaging) messaging_manager_destroy(a->messaging);
        if (a->events) { event_queue_close(a->events); event_queue_destroy(a->events); }
        if (a->receipts) a->receipts->destroy(a->receipts);
        if (a->reactions) a->reactions->destroy(a->reactions);
        if (a->aliases) a->aliases->destroy(a->aliases);
        if (a->contacts) a->contacts->destroy(a->contacts);
        if (a->chats) a->chats->destroy(a->chats);
        if (a->messages) a->messages->destroy(a->messages);
    }
    if (h->exporter) h->exporter->destroy(h->exporter);
    if (h->settings) settings_manager_destroy(h->settings);
    sqlite_database_close(h->db);
    char cmd[128];
    snprintf(cmd, sizeof(cmd), "rm -rf %s", h->dir);
    if (system(cmd) != 0) printf("could not remove %s\n", h->dir);
}

#endif
