/* The chats an agent with access admin may answer its own requests in:
 * switched on in a dialog, kept in the settings. */
#include "tui_app_state.h"
#include "utilities/str_util.h"

#include <ctype.h>
#include <stdio.h>
#include <string.h>

static const Settings *settings(TuiApp *app) { return settings_manager_current(app->deps.settings); }

void tui_app_open_self_chats(TuiApp *app) {
    settings_panel_close(&app->settings_panel);
    chat_toggle_dialog_open(&app->self_chats, "Chats an agent may answer in by itself", "All chats agents may use");
    const char *p = settings(app)->automation_self_chats;
    while (*p) {                                             /* "*" for all, or JIDs, separated by commas */
        while (*p == ',' || isspace((unsigned char)*p)) p++;
        size_t n = strcspn(p, ",");
        char item[128];
        if (n > 0 && n < sizeof(item)) {
            memcpy(item, p, n);
            item[n] = '\0';
            char *jid = str_trim(item);
            if (strcmp(jid, "*") == 0) chat_toggle_dialog_set_all(&app->self_chats, 1);
            else chat_toggle_dialog_set_on(&app->self_chats, jid);
        }
        p += n;
    }
    app->dirty = 1;
}

void tui_app_self_chats_summary(TuiApp *app, char *out, size_t size) {
    const char *p = settings(app)->automation_self_chats;
    int n = 0, all = 0;
    while (*p) {
        while (*p == ',' || isspace((unsigned char)*p)) p++;
        size_t len = strcspn(p, ",");
        if (len > 0) { if (*p == '*') all = 1; else n++; }
        p += len;
    }
    if (all) str_copy(out, size, "every chat agents may use");
    else if (n == 0) str_copy(out, size, "no chat");
    else snprintf(out, size, "%d chat%s", n, n == 1 ? "" : "s");
}

static void save_self_chats(TuiApp *app) {
    const char *jids[CHAT_TOGGLE_CAPACITY];
    int all = chat_toggle_dialog_all(&app->self_chats);
    int n = chat_toggle_dialog_chats(&app->self_chats, jids, CHAT_TOGGLE_CAPACITY);
    Settings s = *settings(app);
    size_t used = 0;
    s.automation_self_chats[0] = '\0';
    if (all) used = (size_t)snprintf(s.automation_self_chats, sizeof(s.automation_self_chats), "*");
    for (int i = 0; i < n; i++) {                            /* kept even under All chats, for when that goes off again */
        if (used + strlen(jids[i]) + 2 >= sizeof(s.automation_self_chats)) {
            tui_app_toast(app, "That is too many chats to keep; switch some off or use All chats", 1);
            return;
        }
        used += (size_t)snprintf(s.automation_self_chats + used, sizeof(s.automation_self_chats) - used, "%s%s", used ? "," : "", jids[i]);
    }
    if (settings_manager_apply(app->deps.settings, &s) != 0) { tui_app_toast(app, "The setting could not be saved", 1); return; }
    char msg[160];
    if (all) snprintf(msg, sizeof(msg), "An admin agent may answer its own sends in every chat agents may use");
    else if (n == 0) snprintf(msg, sizeof(msg), "No chat is answered by an agent itself now");
    else snprintf(msg, sizeof(msg), "An admin agent may answer its own sends in %d chat%s", n, n == 1 ? "" : "s");
    tui_app_toast(app, msg, 0);
}

void tui_app_self_chats_request(TuiApp *app, PopupResult result) {
    app->dirty = 1;
    if (result == POPUP_CHOSEN) save_self_chats(app);
}
