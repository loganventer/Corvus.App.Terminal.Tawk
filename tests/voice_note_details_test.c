/* What an agent is told about a voice note or video: its length, and when a
 * recipient first played it. */
#include "clients/control/control_codec.h"
#include "core/message.h"
#include "engines/played_time.h"
#include "utilities/str_util.h"

#include <stdio.h>
#include <string.h>

static int failures = 0;

#define CHECK(cond, what) do { if (!(cond)) { fprintf(stderr, "FAIL: %s\n", what); failures++; } } while (0)

static Receipt played(int64_t delivered, int64_t read, int64_t at) {
    Receipt r;
    memset(&r, 0, sizeof(r));
    r.delivered_at = delivered;
    r.read_at = read;
    r.played_at = at;
    return r;
}

static void test_which_messages_can_be_played(void) {
    CHECK(played_time_applies(MESSAGE_TYPE_AUDIO), "a voice note can be played");
    CHECK(played_time_applies(MESSAGE_TYPE_VIDEO), "a video can be played");
    CHECK(!played_time_applies(MESSAGE_TYPE_TEXT), "text cannot");
    CHECK(!played_time_applies(MESSAGE_TYPE_IMAGE), "nor an image");
    CHECK(!played_time_applies(MESSAGE_TYPE_DOCUMENT), "nor a document");
}

static void test_the_first_time_played(void) {
    Receipt none[2] = { played(100, 0, 0), played(100, 120, 0) };
    Receipt one[1] = { played(100, 120, 500) };
    Receipt many[3] = { played(100, 120, 900), played(100, 0, 0), played(100, 130, 700) };

    CHECK(played_time_first(none, 2) == 0, "delivered and read is not played");
    CHECK(played_time_first(one, 1) == 500, "one recipient who played it");
    CHECK(played_time_first(many, 3) == 700, "the earliest of several, skipping one who has not");
    CHECK(played_time_first(NULL, 0) == 0, "no receipts at all");
}

static void test_the_length_in_the_message(void) {
    Message m;
    message_init(&m);
    str_copy(m.id, sizeof(m.id), "A1");
    m.type = MESSAGE_TYPE_AUDIO;
    m.duration_s = 42;

    cJSON *o = control_codec_message(&m, "You");
    cJSON *length = cJSON_GetObjectItemCaseSensitive(o, "duration_s");
    CHECK(cJSON_IsNumber(length) && (int)length->valuedouble == 42, "a voice note carries its length");
    CHECK(cJSON_GetObjectItemCaseSensitive(o, "played_at") == NULL, "and no played time of its own");
    cJSON_Delete(o);

    m.duration_s = 0;
    o = control_codec_message(&m, "You");
    CHECK(cJSON_GetObjectItemCaseSensitive(o, "duration_s") == NULL, "no length when it is not known");
    cJSON_Delete(o);

    m.type = MESSAGE_TYPE_TEXT;
    m.duration_s = 42;
    o = control_codec_message(&m, "You");
    CHECK(cJSON_GetObjectItemCaseSensitive(o, "duration_s") == NULL, "text has no length");
    cJSON_Delete(o);

    message_dispose(&m);
}

int main(void) {
    test_which_messages_can_be_played();
    test_the_first_time_played();
    test_the_length_in_the_message();
    if (failures) { fprintf(stderr, "%d check(s) failed\n", failures); return 1; }
    printf("voice note details: all checks passed\n");
    return 0;
}
