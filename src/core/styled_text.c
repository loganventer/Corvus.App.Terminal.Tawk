#include "core/styled_text.h"

#include <stdlib.h>
#include <string.h>

void styled_text_init(StyledText *s) { memset(s, 0, sizeof(*s)); }

void styled_text_dispose(StyledText *s) {
    if (!s) return;
    free(s->text);
    free(s->runs);
    styled_text_init(s);
}

int styled_text_style_at(const StyledText *s, size_t offset) {
    for (int i = 0; s && i < s->run_count; i++) {
        if (offset >= s->runs[i].start && offset < s->runs[i].end) return s->runs[i].style;
    }
    return 0;
}
