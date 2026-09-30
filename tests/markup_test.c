/* WhatsApp's text formatting: which marks count, what the text becomes and
 * how each part is styled. */
#include "core/text_style.h"
#include "engines/whatsapp_markup.h"

#include <stdio.h>
#include <string.h>

static int failures = 0;

#define CHECK(cond, what) do { if (!(cond)) { fprintf(stderr, "FAIL: %s\n", what); failures++; } } while (0)

/* The text shown, and the style of the first byte of `probe` in it. */
static int shown(const char *raw, const char *expect, const char *probe, int style, const char *what) {
    StyledText s;
    if (whatsapp_markup_parse(raw, NULL, 0, &s) != 0) { CHECK(0, what); return 0; }
    int ok = strcmp(s.text, expect) == 0;
    if (ok && probe) {
        const char *at = strstr(s.text, probe);
        ok = at && styled_text_style_at(&s, (size_t)(at - s.text)) == style;
    }
    if (!ok) fprintf(stderr, "  got \"%s\"\n", s.text);
    CHECK(ok, what);
    styled_text_dispose(&s);
    return ok;
}

static void test_marks(void) {
    shown("hello *world*", "hello world", "world", TEXT_STYLE_BOLD, "*bold*");
    shown("_soft_ words", "soft words", "soft", TEXT_STYLE_ITALIC, "_italic_");
    shown("a ~gone~ b", "a gone b", "gone", TEXT_STYLE_STRIKE, "~strike~");
    shown("run `make test` now", "run make test now", "make", TEXT_STYLE_CODE, "`code`");
    shown("*_both_*", "both", "both", TEXT_STYLE_BOLD | TEXT_STYLE_ITALIC, "marks nest");
    shown("(*quiet*)", "(quiet)", "quiet", TEXT_STYLE_BOLD, "punctuation counts as a word edge");
    shown("`*not bold*`", "*not bold*", "not", TEXT_STYLE_CODE, "code keeps its marks");
}

static void test_literal(void) {
    shown("2*3*4", "2*3*4", NULL, 0, "marks inside a word stay");
    shown("a * b * c", "a * b * c", NULL, 0, "marks next to spaces stay");
    shown("*open only", "*open only", NULL, 0, "an unclosed mark stays");
    shown("**", "**", NULL, 0, "an empty pair stays");
    shown("*two\nlines*", "*two\nlines*", NULL, 0, "marks do not cross a line break");
    shown("snake_case_name", "snake_case_name", NULL, 0, "underscores inside words stay");
}

static void test_lines(void) {
    shown("```\nint x = *p;\n```", "int x = *p;", "int", TEXT_STYLE_BLOCK, "a block keeps everything inside it");
    shown("see ```a\nb``` done", "see a\nb done", "a\nb", TEXT_STYLE_BLOCK, "a block can span lines");
    shown("> quoted *bit*\nreply", "\xE2\x96\x8E quoted bit\nreply", "bit", TEXT_STYLE_QUOTE | TEXT_STYLE_BOLD, "quote lines get a bar");
    shown("- milk\n* eggs", "\xE2\x80\xA2 milk\n\xE2\x80\xA2 eggs", NULL, 0, "list items get bullets");
    shown("1. first", "1. first", NULL, 0, "numbered lists stay as typed");
    shown("caf\xC3\xA9 *cr\xC3\xA8me*", "caf\xC3\xA9 cr\xC3\xA8me", "cr", TEXT_STYLE_BOLD, "multi-byte letters");
}

static void test_mentions(void) {
    MentionName names[1] = { { "27820000001", "Lindiwe" } };
    StyledText s;
    whatsapp_markup_parse("thanks @27820000001 and @999", names, 1, &s);
    CHECK(strcmp(s.text, "thanks @Lindiwe and @999") == 0, "a known mention shows the name, others stay");
    const char *at = strstr(s.text, "@Lindiwe");
    CHECK(at && styled_text_style_at(&s, (size_t)(at - s.text)) == TEXT_STYLE_MENTION, "and is styled as a mention");
    styled_text_dispose(&s);

    char plain[128];
    whatsapp_markup_plain("*hi* @27820000001\n> ok", names, 1, plain, sizeof(plain));
    CHECK(strcmp(plain, "hi @Lindiwe \xE2\x96\x8E ok") == 0, "plain text for previews is one line without marks");
    whatsapp_markup_plain("", NULL, 0, plain, sizeof(plain));
    CHECK(plain[0] == '\0', "empty text stays empty");
}

int main(void) {
    test_marks();
    test_literal();
    test_lines();
    test_mentions();
    if (failures == 0) printf("ok: WhatsApp formatting is read as the phone reads it\n");
    return failures != 0;
}
