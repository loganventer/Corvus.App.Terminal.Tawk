/* How the voice notes and videos you sent were received, in the terminal
 * client: where the conversation gets the receipts, and opening and closing
 * the details under a note (a click on its head line, or Alt+D). */
#include "tui_app_state.h"

/* The manager that keeps an account's receipts: the account in view, or the
 * one a message belongs to in a chat merged across accounts. */
static MessagingManager *manager_of(TuiApp *app, AccountId owner) {
    IAccountDirectory *dir = app->deps.directory;
    if (owner != ACCOUNT_ID_NONE && dir) {
        const AccountServices *sv = dir->find(dir, owner);
        if (sv && sv->messaging) return sv->messaging;
    }
    return app->deps.messaging;
}

static int list_receipts(void *ctx, const Message *message, AccountId owner, Receipt *out, int max) {
    MessagingManager *mgr = manager_of(ctx, owner);
    return mgr ? messaging_manager_message_receipts(mgr, message->id, out, max) : 0;
}

void tui_app_init_receipts(TuiApp *app) {
    app->receipt_source = (ReceiptSource){ app, list_receipts };
}

int tui_app_toggle_details(TuiApp *app, int index) {
    if (!message_view_has_details(&app->message_view, index)) return 0;
    int n = 0;
    const Message *msgs = tui_app_messages(app, &n);
    if (index < 0 || index >= n) return 0;
    message_view_hold(&app->message_view, msgs, n);
    message_view_toggle_details(&app->message_view, msgs[index].id);
    app->dirty = 1;
    return 1;
}
