/* Web addresses in messages: finding them, placing them on the rows a long one wraps over, finding
 * the one a click lands on, and opening only web addresses. */
#include "clients/tui/message_links.h"
#include "clients/tui/message_view.h"
#include "engines/url_finder.h"
#include "infrastructure/system_url_opener.h"

#include <stdio.h>
#include <locale.h>
#include <stdlib.h>
#include <string.h>

static int failures = 0;

#define CHECK(cond, what) do { if (!(cond)) { fprintf(stderr, "FAIL: %s\n", what); failures++; } } while (0)

/* Widths need a UTF-8 locale; one is picked whichever the system has. */
static int utf8_locale(void) {
    if (!setlocale(LC_ALL, "") || MB_CUR_MAX == 1) {
        if (!setlocale(LC_ALL, "C.UTF-8")) setlocale(LC_ALL, "en_US.UTF-8");
    }
    return MB_CUR_MAX > 1;
}

static void test_finder(void) {
    const char *text = "a https://one.example/x, and (http://two.example/y) done";
    size_t begin, end;
    CHECK(url_find_next(text, 0, &begin, &end) == 0 && !strncmp(text + begin, "https://one.example/x", end - begin) &&
          end - begin == strlen("https://one.example/x"), "the first address is found without its comma");
    CHECK(url_find_next(text, end, &begin, &end) == 0 && end - begin == strlen("http://two.example/y"), "the next one follows it");
    CHECK(url_find_next(text, end, &begin, &end) != 0, "and then there are no more");
    CHECK(url_find_next(text, strlen(text) + 1, &begin, &end) != 0, "a start past the end finds nothing");
}

static void test_row_spans(void) {
    MessageUrlSpan span[4];
    const char *text = "see https://example.com/a/b/c now";
    int n = message_links_in_text_row(text, 0, (int)strlen(text), 10, 40, span, 4);
    CHECK(n == 1 && span[0].x0 == 14 && span[0].x1 == 14 + (int)strlen("https://example.com/a/b/c") &&
          !strcmp(span[0].url, "https://example.com/a/b/c"), "an address is placed after the text before it");

    /* The same message wrapped: the address starts on the first row and runs on to the second. */
    const char *wrapped = "go https://example.com/long/path/here";
    int cut = 3 + 12;                                        /* the first row ends 12 bytes into the address */
    n = message_links_in_text_row(wrapped, 0, cut, 0, 40, span, 4);
    CHECK(n == 1 && span[0].x0 == 3 && span[0].x1 == cut && !strcmp(span[0].url, "https://example.com/long/path/here"),
          "the first row's part opens the whole address");
    n = message_links_in_text_row(wrapped, cut, (int)strlen(wrapped) - cut, 0, 40, span, 4);
    CHECK(n == 1 && span[0].x0 == 0 && span[0].x1 == (int)strlen(wrapped) - cut && !strcmp(span[0].url, "https://example.com/long/path/here"),
          "so does the second row's");

    n = message_links_in_text_row("no address", 0, 10, 0, 40, span, 4);
    CHECK(n == 0, "a row without one has none");
    if (utf8_locale()) {
        const char *accented = "caf\xC3\xA9 https://a.example";
        n = message_links_in_text_row(accented, 0, (int)strlen(accented), 0, 40, span, 4);
        CHECK(n == 1 && span[0].x0 == 5, "columns, not bytes, place the address after an accent");
    } else {
        fprintf(stderr, "skipped: no UTF-8 locale on this system\n");
    }
    n = message_links_in_text_row("https://a.example/very/long", 0, (int)strlen("https://a.example/very/long"), 0, 10, span, 4);
    CHECK(n == 1 && span[0].x1 == 10, "an address wider than the bubble is cut at its edge");
}

static void test_card_row(void) {
    MessageUrlSpan span;
    CHECK(message_links_on_card_row("https://example.com", "example.com", 5, 30, &span) == 1 && span.x0 == 5 && span.x1 == 16 &&
          !strcmp(span.url, "https://example.com"), "a link card's site line opens the card's address");
    CHECK(message_links_on_card_row("https://example.com", "", 5, 30, &span) == 0, "an empty line opens nothing");
}

static void test_hit(void) {
    MessageView view;
    message_view_init(&view);
    view.url_span_count = 1;
    view.url_spans[0] = (MessageUrlSpan){ 7, 10, 20, "https://example.com" };
    char url[MESSAGE_URL_MAX];
    CHECK(message_view_hit_url(&view, 7, 10, url, sizeof(url)) == 0 && !strcmp(url, "https://example.com"), "a click on the first column hits");
    CHECK(message_view_hit_url(&view, 7, 19, url, sizeof(url)) == 0, "and on the last");
    CHECK(message_view_hit_url(&view, 7, 20, url, sizeof(url)) != 0, "one past the end does not");
    CHECK(message_view_hit_url(&view, 7, 9, url, sizeof(url)) != 0, "nor one before the start");
    CHECK(message_view_hit_url(&view, 8, 12, url, sizeof(url)) != 0, "nor another row");
    message_view_dispose(&view);
}

static void test_opener(void) {
    IUrlOpener *opener = system_url_opener_create();
    CHECK(opener != NULL, "the opener is made");
    if (!opener) return;
    CHECK(opener->open(opener, "file:///etc/passwd") != 0, "a file address is refused");
    CHECK(opener->open(opener, "javascript:alert(1)") != 0, "a script address is refused");
    CHECK(opener->open(opener, "ftp://example.com") != 0, "another scheme is refused");
    CHECK(opener->open(opener, NULL) != 0, "no address is refused");
    opener->destroy(opener);
}

int main(void) {
    test_finder();
    test_row_spans();
    test_card_row();
    test_hit();
    test_opener();
    if (failures) { fprintf(stderr, "%d failure(s)\n", failures); return 1; }
    puts("message links: ok");
    return 0;
}
