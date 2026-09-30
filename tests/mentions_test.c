/* Mentions: finding members while typing "@", turning "@Name" into what
 * WhatsApp sends, reading them back, and notifying when you are mentioned. */
#include "core/chat.h"
#include "core/event.h"
#include "core/settings.h"
#include "engines/mention_encoder.h"
#include "engines/mention_matcher.h"
#include "engines/notification_policy.h"
#include "resource_access/json_protocol.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int failures = 0;

#define CHECK(cond, what) do { if (!(cond)) { fprintf(stderr, "FAIL: %s\n", what); failures++; } } while (0)

static void test_matcher(void) {
    MentionCandidate members[4] = {
        { "1@s.whatsapp.net", "Jan de Wet" }, { "2@s.whatsapp.net", "Lindiwe" },
        { "3@s.whatsapp.net", "Marjan" },     { "4@s.whatsapp.net", "Pieter" },
    };
    MentionCandidate out[8];
    int n = mention_matcher_rank(members, 4, "jan", out, 8);
    CHECK(n == 2 && strcmp(out[0].name, "Jan de Wet") == 0 && strcmp(out[1].name, "Marjan") == 0,
          "a name starting with the typed text comes before one containing it");
    CHECK(mention_matcher_rank(members, 4, "WET", out, 8) == 1, "matching ignores case and looks at every word");
    CHECK(mention_matcher_rank(members, 4, "", out, 8) == 4, "a bare @ lists everyone");
    CHECK(mention_matcher_rank(members, 4, "zz", out, 8) == 0, "nothing fits");
}

static void test_encoder(void) {
    MentionPick picks[2] = { { "27820000001@s.whatsapp.net", "Jan" }, { "27820000002@s.whatsapp.net", "Jan de Wet" } };
    char out[256];
    MentionList list;
    CHECK(mention_encoder_encode("hi @Jan de Wet and @Jan!", picks, 2, out, sizeof(out), &list) == 0 &&
          strcmp(out, "hi @27820000002 and @27820000001!") == 0, "@Name becomes @number, the longest name first");
    CHECK(list.count == 2 && strcmp(list.items[0].user, "27820000002") == 0, "and both are listed");
    CHECK(mention_encoder_encode("no one here", picks, 2, out, sizeof(out), &list) == 0 && list.count == 0 &&
          strcmp(out, "no one here") == 0, "picks deleted from the text are left out");
    CHECK(mention_encoder_encode("@Jan", picks, 1, out, 4, &list) == -1, "too little room is refused");
}

static void test_protocol(void) {
    Event e;
    CHECK(json_protocol_decode("{\"evt\":\"message\",\"id\":\"M\",\"chat\":\"g@g.us\",\"sender\":\"a@s.whatsapp.net\",\"type\":\"text\","
                               "\"text\":\"hi @123\",\"mentions\":[{\"jid\":\"27820000001@s.whatsapp.net\",\"user\":\"123\"},"
                               "{\"jid\":\"bad\",\"user\":\"1\"},{\"jid\":\"x@lid\",\"user\":\"12a\"}],\"mentions_me\":true}", &e) == 0,
          "a message with mentions decodes");
    MentionList list;
    mention_list_parse(&list, e.message.mentions);
    CHECK(list.count == 1 && strcmp(list.items[0].user, "123") == 0 && e.message.mentions_me,
          "only well-formed mentions are kept, and mentioning you is flagged");
    event_dispose(&e);

    MentionList out;
    mention_list_init(&out);
    mention_list_add(&out, "27820000001@s.whatsapp.net", NULL);
    OutgoingText text = { "hi @27820000001", NULL, &out, 0, 0, 0 };
    char *json = json_protocol_encode_send("g@g.us", &text, "ID1");
    CHECK(json && strstr(json, "\"mentions\":[\"27820000001@s.whatsapp.net\"]"), "sending lists the mentioned JIDs");
    free(json);
}

static void test_policy(void) {
    Settings s;
    settings_set_defaults(&s);
    Chat chat;
    chat_init(&chat, "g@g.us");
    chat.is_group = 1;
    chat.is_muted = 1;
    Message msg;
    message_init(&msg);
    CHECK(!notification_policy_should_notify(&s, &chat, &msg, 1, 0), "a muted group stays quiet");
    msg.mentions_me = 1;
    CHECK(notification_policy_should_notify(&s, &chat, &msg, 1, 0), "unless you are mentioned");
    s.do_not_disturb = 1;
    CHECK(!notification_policy_should_notify(&s, &chat, &msg, 1, 0), "do not disturb still wins");
    s.do_not_disturb = 0;
    s.mention_notifications = 0;
    CHECK(!notification_policy_should_notify(&s, &chat, &msg, 1, 0), "and the setting turns it off");
}

int main(void) {
    test_matcher();
    test_encoder();
    test_protocol();
    test_policy();
    if (failures == 0) printf("ok: mentions are suggested, sent, read back and notify through a mute\n");
    return failures != 0;
}
