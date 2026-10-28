#include "clients/tui/message_links.h"
#include "engines/url_finder.h"
#include "utilities/str_util.h"
#include "utilities/utf8_text.h"

#include <string.h>

/* The columns `length` bytes of `s` take. */
static int columns_of(const char *s, size_t length) {
    int columns = 0;
    utf8_fit(s, length, 1 << 20, &columns);
    return columns;
}

int message_links_in_text_row(const char *text, int offset, int length, int x, int room, MessageUrlSpan *out, int max) {
    if (!text || !out || max <= 0 || offset < 0 || length <= 0 || room <= 0) return 0;
    int found = 0;
    size_t from = 0, begin, end;
    while (found < max && url_find_next(text, from, &begin, &end) == 0) {
        from = end;
        if (end - begin >= MESSAGE_URL_MAX) continue;
        size_t shown_begin = begin > (size_t)offset ? begin : (size_t)offset;
        size_t shown_end = end < (size_t)(offset + length) ? end : (size_t)(offset + length);
        if (shown_begin >= shown_end) continue;                 /* this row shows none of it */
        int x0 = columns_of(text + offset, shown_begin - (size_t)offset);
        int x1 = x0 + columns_of(text + shown_begin, shown_end - shown_begin);
        if (x0 >= room) continue;
        if (x1 > room) x1 = room;
        MessageUrlSpan *span = &out[found++];
        span->y = 0;
        span->x0 = x + x0;
        span->x1 = x + x1;
        memcpy(span->url, text + begin, end - begin);
        span->url[end - begin] = '\0';
    }
    return found;
}

int message_links_on_card_row(const char *url, const char *label, int x, int room, MessageUrlSpan *out) {
    if (!url || !label || !out || !label[0] || room <= 0 || strlen(url) >= MESSAGE_URL_MAX) return 0;
    int columns = utf8_columns(label);
    if (columns > room) columns = room;
    out->y = 0;
    out->x0 = x;
    out->x1 = x + columns;
    str_copy(out->url, sizeof(out->url), url);
    return 1;
}
