/* The line tawk gives the Touch ID sheet for each reason it asks: there is
 * one for every purpose, and it is plain text with nothing filled in. */
#include "core/unlock_purpose.h"

#include <stdio.h>
#include <string.h>

static int failures;
#define CHECK(cond, what) do { if (!(cond)) { printf("FAIL: %s\n", what); failures++; } } while (0)

int main(void) {
    for (int i = 0; i < UNLOCK_PURPOSE_COUNT; i++) {
        const char *reason = unlock_purpose_reason((UnlockPurpose)i);
        CHECK(reason && reason[0] != '\0', "every purpose has a reason");
        CHECK(reason && !strchr(reason, '%') && !strchr(reason, '{'), "and it has no placeholder for a chat name");
        CHECK(reason && reason[strlen(reason) - 1] != '.', "and reads as the end of the system's sentence");
    }
    CHECK(strcmp(unlock_purpose_reason(UNLOCK_FOR_OPEN), unlock_purpose_reason(UNLOCK_FOR_EXPORT)) != 0, "each purpose says what it is for");

    if (failures) { printf("%d failure(s)\n", failures); return 1; }
    printf("ok: the reasons given to Touch ID\n");
    return 0;
}
