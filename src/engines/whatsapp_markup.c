#include "engines/whatsapp_markup.h"
#include "core/text_style.h"
#include "utilities/str_util.h"

#include <ctype.h>
#include <stdlib.h>
#include <string.h>

#define QUOTE_BAR "\xE2\x96\x8E "   /* ▎ in place of "> " */
#define BULLET    "\xE2\x80\xA2 "   /* • in place of "- " or "* " */

typedef struct Builder {
    StyledText *out;
    size_t      length, cap;
    int         run_cap;
    int         failed;
    const MentionName *names;
    int         name_count;
} Builder;

static void append(Builder *b, const char *bytes, size_t n, int style) {
    if (b->failed || n == 0) return;
    if (b->length + n + 1 > b->cap) {
        size_t cap = b->cap ? b->cap : 64;
        while (cap < b->length + n + 1) cap *= 2;
        char *grown = realloc(b->out->text, cap);
        if (!grown) { b->failed = 1; return; }
        b->out->text = grown;
        b->cap = cap;
    }
    memcpy(b->out->text + b->length, bytes, n);
    StyledText *s = b->out;
    if (s->run_count > 0 && s->runs[s->run_count - 1].style == style && s->runs[s->run_count - 1].end == b->length) {
        s->runs[s->run_count - 1].end += n;
    } else {
        if (s->run_count == b->run_cap) {
            int cap = b->run_cap ? b->run_cap * 2 : 8;
            StyledRun *grown = realloc(s->runs, sizeof(StyledRun) * (size_t)cap);
            if (!grown) { b->failed = 1; return; }
            s->runs = grown;
            b->run_cap = cap;
        }
        s->runs[s->run_count++] = (StyledRun){ b->length, b->length + n, style };
    }
    b->length += n;
    b->out->text[b->length] = '\0';
}

/* Word edges: the start or end of the text, white space, or ASCII punctuation. */
static int edge(const char *raw, long i, size_t len) {
    if (i < 0 || (size_t)i >= len) return 1;
    unsigned char c = (unsigned char)raw[i];
    return isspace(c) || (c < 128 && ispunct(c));
}

static int is_mark(char c) { return c == '*' || c == '_' || c == '~' || c == '`'; }

static int mark_style(char c) {
    switch (c) {
        case '*': return TEXT_STYLE_BOLD;
        case '_': return TEXT_STYLE_ITALIC;
        case '~': return TEXT_STYLE_STRIKE;
        default:  return TEXT_STYLE_CODE;
    }
}

/* Plain text, with "@<digits>" of a known mention shown as "@<name>". */
static void literal(Builder *b, const char *raw, size_t from, size_t to, int style) {
    size_t seg = from;
    for (size_t i = from; i < to; i++) {
        if (raw[i] != '@' || i + 1 >= to || !isdigit((unsigned char)raw[i + 1])) continue;
        size_t j = i + 1;
        while (j < to && isdigit((unsigned char)raw[j])) j++;
        for (int k = 0; k < b->name_count; k++) {
            if (strlen(b->names[k].user) != j - i - 1 || strncmp(b->names[k].user, raw + i + 1, j - i - 1) != 0) continue;
            append(b, raw + seg, i - seg, style);
            append(b, "@", 1, style | TEXT_STYLE_MENTION);
            append(b, b->names[k].name, strlen(b->names[k].name), style | TEXT_STYLE_MENTION);
            seg = j;
            i = j - 1;
            break;
        }
    }
    append(b, raw + seg, to - seg, style);
}

/* The closing mark for an opening one at `open`, on the same stretch, or -1. */
static long closing(const char *raw, size_t open, size_t to, size_t len) {
    char c = raw[open];
    for (size_t j = open + 2; j < to; j++) {
        if (raw[j] != c) continue;
        if (isspace((unsigned char)raw[j - 1])) continue;
        if (!edge(raw, (long)j + 1, len) || (j + 1 < len && raw[j + 1] == c)) continue;
        return (long)j;
    }
    return -1;
}

/* Inline marks in raw[from, to), which holds no line break. */
static void inline_marks(Builder *b, const char *raw, size_t from, size_t to, size_t len, int style) {
    size_t seg = from;
    for (size_t i = from; i < to; i++) {
        char c = raw[i];
        if (!is_mark(c)) continue;
        int opens = edge(raw, (long)i - 1, len) && (i == 0 || raw[i - 1] != c) && i + 1 < to && !isspace((unsigned char)raw[i + 1]);
        if (!opens) continue;
        long close = closing(raw, i, to, len);
        if (close < 0) continue;
        literal(b, raw, seg, i, style);
        if (c == '`') append(b, raw + i + 1, (size_t)close - i - 1, style | TEXT_STYLE_CODE);
        else inline_marks(b, raw, i + 1, (size_t)close, len, style | mark_style(c));
        i = (size_t)close;
        seg = i + 1;
    }
    literal(b, raw, seg, to, style);
}

static const char *find(const char *raw, size_t from, size_t len, const char *what) {
    const char *hit = from < len ? strstr(raw + from, what) : NULL;
    return hit && (size_t)(hit - raw) < len ? hit : NULL;
}

int whatsapp_markup_parse(const char *raw, const MentionName *names, int name_count, StyledText *out) {
    styled_text_init(out);
    Builder b = { out, 0, 0, 0, 0, names, name_count };
    if (!raw) raw = "";
    size_t len = strlen(raw), pos = 0;
    while (pos < len && !b.failed) {
        const char *nl = strchr(raw + pos, '\n');
        size_t eol = nl ? (size_t)(nl - raw) : len;
        int style = 0;
        int line_start = pos == 0 || raw[pos - 1] == '\n';
        if (line_start && strncmp(raw + pos, "> ", 2) == 0) {
            append(&b, QUOTE_BAR, strlen(QUOTE_BAR), TEXT_STYLE_QUOTE);
            style = TEXT_STYLE_QUOTE;
            pos += 2;
        } else if (line_start && (strncmp(raw + pos, "- ", 2) == 0 || strncmp(raw + pos, "* ", 2) == 0)) {
            append(&b, BULLET, strlen(BULLET), 0);
            pos += 2;
        }
        /* A ``` block may start anywhere and run over several lines. */
        const char *fence = find(raw, pos, eol, "```");
        const char *fence_end = fence ? find(raw, (size_t)(fence - raw) + 3, len, "```") : NULL;
        if (fence && fence_end) {
            size_t start = (size_t)(fence - raw), stop = (size_t)(fence_end - raw);
            inline_marks(&b, raw, pos, start, len, style);
            size_t inner = start + 3;
            if (inner < stop && raw[inner] == '\n') inner++;          /* a block on its own lines */
            size_t inner_end = stop;
            if (inner_end > inner && raw[inner_end - 1] == '\n') inner_end--;
            append(&b, raw + inner, inner_end - inner, TEXT_STYLE_BLOCK);
            pos = stop + 3;
            continue;
        }
        inline_marks(&b, raw, pos, eol, len, style);
        if (eol < len) append(&b, "\n", 1, 0);
        pos = eol + 1;
    }
    if (!b.failed && !out->text) out->text = str_dup("");
    if (b.failed || !out->text) { styled_text_dispose(out); return -1; }
    return 0;
}

void whatsapp_markup_plain(const char *raw, const MentionName *names, int name_count, char *out, size_t size) {
    if (!size) return;
    out[0] = '\0';
    StyledText s;
    if (whatsapp_markup_parse(raw, names, name_count, &s) != 0) {
        str_copy(out, size, raw ? raw : "");
    } else {
        str_copy(out, size, s.text ? s.text : "");
        styled_text_dispose(&s);
    }
    for (char *p = out; *p; p++) if (*p == '\n' || *p == '\r' || *p == '\t') *p = ' ';
}
