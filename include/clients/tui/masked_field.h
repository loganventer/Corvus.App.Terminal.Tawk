#ifndef APP_CLIENTS_TUI_MASKED_FIELD_H
#define APP_CLIENTS_TUI_MASKED_FIELD_H

#include <stddef.h>

#include "core/passphrase.h"

/* A line where a secret is typed: it keeps UTF-8, takes backspace a whole
 * character at a time and Ctrl+U to clear, and shows how much was typed,
 * never what. Enter is left to whoever holds it. */
typedef struct MaskedField {
    char text[PASSPHRASE_MAX + 1];
    int  length;
} MaskedField;

/* Returns 1 when the key changed the field, 0 when the field does not take it. */
int  masked_field_key(MaskedField *field, int is_key, int ch);
/* The number of characters typed. */
int  masked_field_chars(const MaskedField *field);
/* One '*' per character, at most `max`, or "_" when nothing is typed. */
void masked_field_mask(const MaskedField *field, int max, char *out, size_t size);
/* Hands over what was typed and wipes the field. */
void masked_field_take(MaskedField *field, Passphrase *out);
void masked_field_wipe(MaskedField *field);

#endif
