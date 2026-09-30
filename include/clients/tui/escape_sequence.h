#ifndef APP_CLIENTS_TUI_ESCAPE_SEQUENCE_H
#define APP_CLIENTS_TUI_ESCAPE_SEQUENCE_H

#include "clients/tui/escape_sequence_kind.h"

#define ESCAPE_MOD_SHIFT 1
#define ESCAPE_MOD_ALT   2
#define ESCAPE_MOD_CTRL  4

/* What followed an ESC. For keys, `code` is the character and `mods` a set
 * of ESCAPE_MOD_* bits. */
typedef struct EscapeSequence {
    EscapeSequenceKind kind;
    int                code;
    int                mods;
} EscapeSequence;

/* Called after curses returned ESC: reads the rest of a control sequence if
 * one is waiting. Recognises the bracketed-paste start and modified keys in
 * the xterm "modifyOtherKeys" form (CSI 27;m;c~) and the "CSI c;m u" form,
 * so Ctrl+Shift+L can be told from Ctrl+L. Anything else is pushed back for
 * curses to read as usual. */
EscapeSequence escape_sequence_read(void);
/* The terminal requests that make modified keys arrive in those forms. */
const char    *escape_sequence_enable(void);
const char    *escape_sequence_disable(void);

#endif
