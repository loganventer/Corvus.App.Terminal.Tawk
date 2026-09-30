#ifndef APP_CLIENTS_TUI_SLASH_COMMAND_H
#define APP_CLIENTS_TUI_SLASH_COMMAND_H

typedef struct TuiApp TuiApp;

/* Runs a command; args is the text after the command name (may be ""). */
typedef void (*SlashCommandHandler)(TuiApp *app, const char *args);

/* One /command typed in the input line. */
typedef struct SlashCommand {
    const char         *name;       /* without the slash */
    const char         *args;       /* argument hint, e.g. "[8h|1w|always]" */
    const char         *help;
    SlashCommandHandler run;
} SlashCommand;

#endif
