#ifndef APP_CLIENTS_TUI_AGENTS_PANEL_H
#define APP_CLIENTS_TUI_AGENTS_PANEL_H

#include "clients/tui/agents_panel_model.h"
#include "clients/tui/agents_panel_request.h"
#include "clients/tui/agents_view.h"
#include "clients/tui/text_caret.h"
#include "clients/tui/text_field.h"
#include "clients/tui/ui_rect.h"

/* The Agents tab: an admin panel for programs reaching tawk through the
 * control socket. Queue shows requests waiting for you with their risk in
 * words, a mark and a colour, a countdown, and the full text; a approves,
 * e edits first, s allows the same again for the session, d declines,
 * space marks several. HIGH requests (deletes, blocks) ignore a: they need
 * Shift+A and then Y in a warning that starts on Keep. Agents lists who is
 * connected (x disconnects, p pauses, r forgets allowances), Log shows
 * what they did, and Permissions changes what they may do. The panel
 * never acts itself: its owner carries out what it asks for. */
typedef struct AgentsPanel {
    int        open;
    AgentsView view;
    int        selected[AGENTS_VIEW_COUNT];
    int        scroll[AGENTS_VIEW_COUNT];
    int        marks[APPROVAL_QUEUE_MAX];      /* ids of marked requests */
    int        mark_count;
    int        editing;                        /* changing the text before approving */
    int        editing_setting;                /* typing a permission's value */
    TextField  edit;
    int        confirming;                     /* id of the HIGH request asking for Y, or 0 */
    int        confirm_on_action;              /* the action button, not Keep, is selected */
    int        expanded;                       /* Enter: a taller detail pane */
    int        log_reads;                      /* show reads in the log */
    int        log_filter;                     /* 0 all, 1 allowed, 2 declined, 3 refused or failed */
    int        searching;
    TextField  search;
    char       hint[160];                      /* a word about the last key, such as "HIGH needs Shift+A" */
    TextCaret  caret;
    /* What the last request is about, for the owner. */
    int        chosen[APPROVAL_QUEUE_MAX];
    int        chosen_count;
    int        chosen_approve;
    int        chosen_remember;
    int        chosen_edited;
    int        chosen_conn;
    char       setting_key[32];
    char       setting_value[600];
    UiRect     last_rect;
    UiRect     tab_rects[AGENTS_VIEW_COUNT];
    UiRect     list_rect;
} AgentsPanel;

void               agents_panel_open(AgentsPanel *panel, AgentsView view);
AgentsPanelRequest agents_panel_key(AgentsPanel *panel, const AgentsPanelModel *model, int is_key_code, int ch);
AgentsPanelRequest agents_panel_click(AgentsPanel *panel, const AgentsPanelModel *model, int y, int x);
void               agents_panel_paste(AgentsPanel *panel, const char *utf8);
/* The text as you edited it, after APPROVE with chosen_edited; the caller frees it. */
char              *agents_panel_edited_text(const AgentsPanel *panel);
void               agents_panel_render(AgentsPanel *panel, UiRect area, const AgentsPanelModel *model);

#endif
