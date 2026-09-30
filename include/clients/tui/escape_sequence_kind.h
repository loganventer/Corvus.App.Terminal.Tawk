#ifndef APP_CLIENTS_TUI_ESCAPE_SEQUENCE_KIND_H
#define APP_CLIENTS_TUI_ESCAPE_SEQUENCE_KIND_H

typedef enum EscapeSequenceKind {
    ESCAPE_SEQUENCE_NONE = 0,     /* a plain ESC, or something curses will read itself */
    ESCAPE_SEQUENCE_PASTE,        /* start of a bracketed paste */
    ESCAPE_SEQUENCE_KEY           /* a key with modifiers curses cannot report */
} EscapeSequenceKind;

#endif
