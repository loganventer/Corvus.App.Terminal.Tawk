#ifndef APP_CLIENTS_TUI_MENTION_SUGGESTIONS_H
#define APP_CLIENTS_TUI_MENTION_SUGGESTIONS_H

#include "clients/tui/ui_rect.h"
#include "core/mention_candidate.h"

#define MENTION_SUGGESTIONS_MAX 8

/* The group members that fit an "@name" being typed, listed above the
 * input: ↑↓ or Tab choose, Enter or a click picks, Esc hides the list
 * until another "@" is typed. */
typedef struct MentionSuggestions {
    MentionCandidate items[MENTION_SUGGESTIONS_MAX];
    int              count;
    int              selected;
    int              start;        /* where the "@" is in the input */
    int              end;          /* the cursor, where the typed name ends */
    int              dismissed;    /* start + 1 of the "@" the user hid the list for */
    UiRect           rows[MENTION_SUGGESTIONS_MAX];
} MentionSuggestions;

/* Shows `items` for the "@" at `start`; the selection stays while the same one is typed. */
void mention_suggestions_open(MentionSuggestions *s, const MentionCandidate *items, int count, int start, int end);
void mention_suggestions_close(MentionSuggestions *s);
void mention_suggestions_dismiss(MentionSuggestions *s);
void mention_suggestions_move(MentionSuggestions *s, int delta);
const MentionCandidate *mention_suggestions_selected(const MentionSuggestions *s);
/* The row index at (y, x), or -1. */
int  mention_suggestions_hit(const MentionSuggestions *s, int y, int x);
/* Draws the list so its bottom sits just above `anchor` (the input). */
void mention_suggestions_render(MentionSuggestions *s, UiRect anchor);

#endif
