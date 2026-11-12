#include "clients/tui/masked_field.h"
#include "utilities/secure_zero.h"

#include <ncurses.h>
#include <string.h>

#ifndef CTRL
#define CTRL(c) ((c) & 0x1f)
#endif

void masked_field_wipe(MaskedField *f) {
    secure_zero(f, sizeof(*f));
}

void masked_field_take(MaskedField *f, Passphrase *out) {
    f->text[f->length] = '\0';
    if (passphrase_set(out, f->text) != 0) passphrase_wipe(out);
    masked_field_wipe(f);
}

int masked_field_chars(const MaskedField *f) {
    int count = 0;
    for (int i = 0; i < f->length; i++) if (((unsigned char)f->text[i] & 0xC0) != 0x80) count++;
    return count;
}

void masked_field_mask(const MaskedField *f, int max, char *out, size_t size) {
    if (size == 0) return;
    int count = masked_field_chars(f);
    if (count > max) count = max;
    size_t used = 0;
    for (int i = 0; i < count && used + 1 < size; i++) out[used++] = '*';
    if (used == 0 && size > 1) out[used++] = '_';
    out[used] = '\0';
}

int masked_field_key(MaskedField *f, int is_key, int ch) {
    if ((is_key && ch == KEY_BACKSPACE) || (!is_key && (ch == 127 || ch == 8))) {
        /* A whole character goes, not one byte of it. */
        while (f->length > 0 && ((unsigned char)f->text[f->length - 1] & 0xC0) == 0x80) f->length--;
        if (f->length > 0) f->length--;
        f->text[f->length] = '\0';
        return 1;
    }
    if (!is_key && ch == CTRL('u')) { masked_field_wipe(f); return 1; }
    if (is_key || ch < 32) return 0;
    /* The character as UTF-8, as the secret was typed when it was set. */
    char utf8[5];
    int n = 0;
    unsigned int c = (unsigned int)ch;
    if (c < 0x80) utf8[n++] = (char)c;
    else if (c < 0x800) { utf8[n++] = (char)(0xC0 | (c >> 6)); utf8[n++] = (char)(0x80 | (c & 0x3F)); }
    else if (c < 0x10000) { utf8[n++] = (char)(0xE0 | (c >> 12)); utf8[n++] = (char)(0x80 | ((c >> 6) & 0x3F)); utf8[n++] = (char)(0x80 | (c & 0x3F)); }
    else { utf8[n++] = (char)(0xF0 | (c >> 18)); utf8[n++] = (char)(0x80 | ((c >> 12) & 0x3F)); utf8[n++] = (char)(0x80 | ((c >> 6) & 0x3F)); utf8[n++] = (char)(0x80 | (c & 0x3F)); }
    if (f->length + n > PASSPHRASE_MAX) return 0;
    memcpy(f->text + f->length, utf8, (size_t)n);
    f->length += n;
    return 1;
}
