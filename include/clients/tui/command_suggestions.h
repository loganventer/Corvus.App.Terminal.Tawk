#ifndef APP_CLIENTS_TUI_COMMAND_SUGGESTIONS_H
#define APP_CLIENTS_TUI_COMMAND_SUGGESTIONS_H

#include "clients/tui/slash_command.h"
#include "clients/tui/ui_rect.h"

#define COMMAND_SUGGESTIONS_MAX 24

/* The list of matching /commands shown above the input while typing one. */
typedef struct CommandSuggestions {
    int selected;
    int matches[COMMAND_SUGGESTIONS_MAX];
    int count;
} CommandSuggestions;

/* Recomputes matches for the typed text ("/mu" -> mute, ...). Returns the count. */
int  command_suggestions_update(CommandSuggestions *s, const SlashCommand *all, int total, const char *typed);
void command_suggestions_move(CommandSuggestions *s, int delta);
/* Selected command, or NULL. */
const SlashCommand *command_suggestions_selected(const CommandSuggestions *s, const SlashCommand *all);
/* Draws the list so its bottom sits just above `anchor` (the input). */
void command_suggestions_render(const CommandSuggestions *s, const SlashCommand *all, UiRect anchor);

#endif
