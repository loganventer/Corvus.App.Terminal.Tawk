/* Mentioning people while typing in a group: the member list that follows
 * an "@", picking from it, and sending with the mentions. */
#include "tui_app_state.h"
#include "core/contact_profile.h"
#include "utilities/str_util.h"

#include <ncurses.h>
#include <stdlib.h>
#include <string.h>

#define MAX_MEMBERS 256

/* The group's members other than you, named as you know them. */
static int group_members(TuiApp *app, const char *group, MentionCandidate *out, int max) {
    ContactProfile p;
    if (profile_manager_details(app->deps.profiles, group, 0, &p) != 0) return 0;
    const char *me = messaging_manager_user_jid(app->deps.messaging);
    int n = 0;
    for (const char *line = p.participants; line && *line && n < max;) {
        const char *end = strchr(line, '\n');
        size_t len = end ? (size_t)(end - line) : strlen(line);
        const char *tab = memchr(line, '\t', len);
        size_t jl = tab ? (size_t)(tab - line) : len;
        if (jl > 0 && jl < sizeof(out[n].jid)) {
            memcpy(out[n].jid, line, jl);
            out[n].jid[jl] = '\0';
            if (!me || strcmp(out[n].jid, me) != 0) {
                messaging_manager_display_name(app->deps.messaging, out[n].jid, out[n].name, sizeof(out[n].name));
                n++;
            }
        }
        line = end ? end + 1 : NULL;
    }
    contact_profile_dispose(&p);
    return n;
}

void tui_app_forget_mentions(TuiApp *app) {
    app->mention_pick_count = 0;
    mention_suggestions_close(&app->mention_suggestions);
    app->mention_suggestions.dismissed = 0;
}

/* Keeps the list in step with the "@name" at the cursor. */
void tui_app_refresh_mention(TuiApp *app) {
    MentionSuggestions *s = &app->mention_suggestions;
    const char *chat = messaging_manager_open_jid(app->deps.messaging);
    char query[128];
    int start = 0;
    if (!chat[0] || !chat_jid_is_group(chat) || !composer_view_mention(&app->composer, query, sizeof(query), &start)) {
        mention_suggestions_close(s);
        s->dismissed = 0;
        return;
    }
    if (start + 1 == s->dismissed) return;
    MentionCandidate *members = calloc(MAX_MEMBERS, sizeof(*members));
    if (!members) return;
    int count = group_members(app, chat, members, MAX_MEMBERS);
    MentionCandidate fit[MENTION_SUGGESTIONS_MAX];
    int n = messaging_manager_rank_mentions(app->deps.messaging, members, count, query, fit, MENTION_SUGGESTIONS_MAX);
    free(members);
    if (n > 0) mention_suggestions_open(s, fit, n, start, app->composer.cursor);
    else mention_suggestions_close(s);
}

/* Replaces the "@name" being typed with the member's full name, and
 * remembers who it was for when the message is sent. */
void tui_app_pick_mention(TuiApp *app, int index) {
    MentionSuggestions *s = &app->mention_suggestions;
    if (index >= 0 && index < s->count) s->selected = index;
    const MentionCandidate *who = mention_suggestions_selected(s);
    if (!who) return;
    char text[160];
    snprintf(text, sizeof(text), "@%s ", who->name);
    if (composer_view_replace(&app->composer, s->start, s->end, text) && app->mention_pick_count < MENTION_LIST_MAX) {
        MentionPick *pick = &app->mention_picks[app->mention_pick_count++];
        str_copy(pick->jid, sizeof(pick->jid), who->jid);
        str_copy(pick->name, sizeof(pick->name), who->name);
    }
    mention_suggestions_close(s);
    app->dirty = 1;
}

/* Keys while the list is open. Returns 1 when the key was used; typing
 * goes on to the input and the list follows it. */
int tui_app_mention_key(TuiApp *app, int is_key, int ch) {
    MentionSuggestions *s = &app->mention_suggestions;
    if (!s->count) return 0;
    if ((is_key && ch == KEY_DOWN) || (!is_key && ch == '\t')) { mention_suggestions_move(s, 1); return 1; }
    if (is_key && (ch == KEY_UP || ch == KEY_BTAB)) { mention_suggestions_move(s, -1); return 1; }
    if ((!is_key && (ch == '\n' || ch == '\r')) || (is_key && ch == KEY_ENTER)) { tui_app_pick_mention(app, -1); return 1; }
    if (!is_key && ch == 27) { mention_suggestions_dismiss(s); return 1; }
    return 0;
}
