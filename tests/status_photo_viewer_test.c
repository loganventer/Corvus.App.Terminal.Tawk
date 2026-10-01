/* Opening a status photo full size shows that photo, not its author's
 * profile picture: only a profile picture is kept up to date by the app. */
#include "clients/tui/image_viewer.h"

#include <stdio.h>
#include <string.h>

static int failures = 0;

#define CHECK(cond, what) do { if (!(cond)) { fprintf(stderr, "FAIL: %s\n", what); failures++; } } while (0)

int main(void) {
    ImageViewer v;
    memset(&v, 0, sizeof(v));
    CHECK(image_viewer_profile_jid(&v) == NULL, "a closed viewer shows no profile picture");

    image_viewer_open_portrait(&v, "27820000000@s.whatsapp.net", "Mom", "/cache/pic-mom.jpg");
    CHECK(v.open && v.portrait && !v.file, "a profile picture opens as a portrait");
    CHECK(image_viewer_profile_jid(&v) && strcmp(image_viewer_profile_jid(&v), "27820000000@s.whatsapp.net") == 0,
          "the app keeps a profile picture sharp by its JID");

    image_viewer_open_file(&v, "STATUS123", "Mom", "status", "/cache/status-photo.jpg");
    CHECK(v.open && v.file && strcmp(v.portrait_path, "/cache/status-photo.jpg") == 0, "a status photo opens as the file it is");
    CHECK(strcmp(v.file_label, "status") == 0 && strcmp(v.portrait_name, "Mom") == 0, "it is titled with the name and what it is");
    CHECK(image_viewer_profile_jid(&v) == NULL, "a status photo is never swapped for a profile picture");

    image_viewer_open_portrait(&v, "27820000000@s.whatsapp.net", "Mom", "/cache/pic-mom.jpg");
    CHECK(!v.file && image_viewer_profile_jid(&v) != NULL, "opening a profile picture afterwards is a portrait again");

    if (failures) return 1;
    printf("ok: a status photo opens as itself, not as its author's profile picture\n");
    return 0;
}
