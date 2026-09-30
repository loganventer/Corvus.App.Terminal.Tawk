#include "clients/tui/escape_sequence.h"

#include <ctype.h>
#include <ncurses.h>
#include <stdlib.h>

#define MAX_SEQ 24

const char *escape_sequence_enable(void)  { return "\033[>4;1m"; }   /* modifyOtherKeys, level 1 */
const char *escape_sequence_disable(void) { return "\033[>4;0m"; }

static void push_back(const wint_t *got, int n) {
    for (int i = n - 1; i >= 0; i--) unget_wch((wchar_t)got[i]);
}

/* "27;6;76~" or "76;6u": the parameters of a modified key. */
static int parse_key(const wint_t *s, int n, EscapeSequence *out) {
    int params[3] = { 0, 0, 0 }, count = 0, digits = 0;
    for (int i = 1; i < n - 1; i++) {                  /* skip '[' and the final byte */
        if (s[i] >= '0' && s[i] <= '9') { params[count] = params[count] * 10 + (int)(s[i] - '0'); digits = 1; }
        else if (s[i] == ';' && count < 2 && digits) { count++; digits = 0; }
        else return 0;
    }
    count += digits;
    int code = 0, mods = 0;
    if (s[n - 1] == '~' && count == 3 && params[0] == 27) { mods = params[1]; code = params[2]; }
    else if (s[n - 1] == 'u' && count == 2) { code = params[0]; mods = params[1]; }
    else return 0;
    if (code <= 0 || code > 0x10FFFF || mods < 1) return 0;
    out->kind = ESCAPE_SEQUENCE_KEY;
    out->code = code;
    out->mods = mods - 1;
    return 1;
}

EscapeSequence escape_sequence_read(void) {
    EscapeSequence out = { ESCAPE_SEQUENCE_NONE, 0, 0 };
    wint_t got[MAX_SEQ];
    int n = 0;
    wtimeout(stdscr, 5);
    while (n < MAX_SEQ) {
        wint_t wc;
        int rc = wget_wch(stdscr, &wc);
        if (rc == ERR) break;
        if (rc == KEY_CODE_YES) { unget_wch((wchar_t)wc); break; }
        got[n++] = wc;
        if (n == 1 && wc != '[') break;                          /* not a control sequence */
        if (n > 1 && wc >= 0x40 && wc <= 0x7E) break;            /* final byte */
    }
    wtimeout(stdscr, 100);
    if (n == 5 && got[0] == '[' && got[1] == '2' && got[2] == '0' && got[3] == '0' && got[4] == '~') {
        out.kind = ESCAPE_SEQUENCE_PASTE;
        return out;
    }
    if (n >= 4 && got[0] == '[' && parse_key(got, n, &out)) return out;
    push_back(got, n);
    return out;
}
