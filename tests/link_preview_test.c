/* Link previews: cards on incoming messages, the card made for a message
 * you sent, and asking for a preview only when the user allows it. */
#include "core/event.h"
#include "resource_access/json_protocol.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int failures = 0;

#define CHECK(cond, what) do { if (!(cond)) { fprintf(stderr, "FAIL: %s\n", what); failures++; } } while (0)

int main(void) {
    Event e;
    CHECK(json_protocol_decode("{\"evt\":\"message\",\"id\":\"L1\",\"chat\":\"a@s.whatsapp.net\",\"sender\":\"a@s.whatsapp.net\","
                               "\"type\":\"text\",\"text\":\"look https://example.com/x\","
                               "\"link\":{\"url\":\"https://example.com/x\",\"title\":\"Example\",\"desc\":\"A page\"},"
                               "\"thumb\":\"/9j/AAAA\"}", &e) == 0, "a message with a link card decodes");
    CHECK(e.message.link && strcmp(e.message.link->title, "Example") == 0 && strcmp(e.message.link->description, "A page") == 0,
          "the card's title and description are kept");
    CHECK(e.message.thumbnail_len > 0, "and its picture");
    event_dispose(&e);

    CHECK(json_protocol_decode("{\"evt\":\"message\",\"id\":\"L2\",\"chat\":\"a@s.whatsapp.net\",\"type\":\"text\",\"text\":\"x\","
                               "\"link\":{\"url\":\"javascript:alert(1)\",\"title\":\"Bad\"}}", &e) == 0 && e.message.link == NULL,
          "only web addresses make a card");
    event_dispose(&e);

    CHECK(json_protocol_decode("{\"evt\":\"link\",\"id\":\"MINE\",\"url\":\"https://example.com\",\"title\":\"T\",\"desc\":\"D\"}", &e) == 0 &&
          e.type == EVENT_LINK_PREVIEW && strcmp(e.message.id, "MINE") == 0 && e.message.link, "the card for your own message decodes");
    event_dispose(&e);

    OutgoingText plain = { "see https://example.com", NULL, NULL, 0, 0, 0 };
    char *json = json_protocol_encode_send("a@s.whatsapp.net", &plain, "ID");
    CHECK(json && !strstr(json, "link_preview"), "without permission no preview is asked for");
    free(json);
    OutgoingText allowed = { "see https://example.com", NULL, NULL, 0, 0, 1 };
    json = json_protocol_encode_send("a@s.whatsapp.net", &allowed, "ID");
    CHECK(json && strstr(json, "\"link_preview\":true"), "with it the backend is asked to fetch one");
    free(json);

    if (failures == 0) printf("ok: link cards are shown, and fetched only when allowed\n");
    return failures != 0;
}
