#ifndef APP_CONTRACTS_I_TERMINAL_TITLE_H
#define APP_CONTRACTS_I_TERMINAL_TITLE_H

/* Tab progress indicator (Windows Terminal); ignored elsewhere. */
typedef enum TerminalProgress {
    TERMINAL_PROGRESS_NONE = 0,
    TERMINAL_PROGRESS_BUSY,     /* indeterminate spinner */
    TERMINAL_PROGRESS_ERROR     /* red */
} TerminalProgress;

typedef struct ITerminalTitle {
    void *ctx;
    void (*set)(struct ITerminalTitle *self, const char *title);
    /* Asks the terminal to draw attention (taskbar flash on Windows Terminal). */
    void (*alert)(struct ITerminalTitle *self);
    void (*progress)(struct ITerminalTitle *self, TerminalProgress state);
    void (*destroy)(struct ITerminalTitle *self);
} ITerminalTitle;

#endif
