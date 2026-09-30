#ifndef APP_ENGINES_WHATSAPP_MARKUP_H
#define APP_ENGINES_WHATSAPP_MARKUP_H

#include <stddef.h>

#include "core/mention_name.h"
#include "core/styled_text.h"

/* WhatsApp's text formatting: *bold*, _italic_, ~strikethrough~, `code`,
 * ```monospace blocks```, "> " quotes and "- " or "* " lists. Marks count
 * only where WhatsApp's apps take them: an opening mark at the start of a
 * word, a closing one at its end, both on the same line; anything else is
 * shown as typed. "@<digits>" of a known mention becomes "@<name>". The
 * result owns its text and runs (styled_text_dispose). Returns 0, or -1
 * when out of memory. */
int  whatsapp_markup_parse(const char *raw, const MentionName *names, int name_count, StyledText *out);
/* The same text flattened to one line without marks, for chat previews,
 * notifications and search results. */
void whatsapp_markup_plain(const char *raw, const MentionName *names, int name_count, char *out, size_t size);

#endif
