#include "infrastructure/audio/ffplay_fallback.h"
#include "utilities/process_util.h"

int ffplay_fallback_add(ArgvBuilder *b, const char *path) {
    if (!process_on_path("ffplay")) return -1;
    argv_builder_add(b, "ffplay");
    argv_builder_add(b, "-nodisp");
    argv_builder_add(b, "-autoexit");
    argv_builder_add(b, "-loglevel");
    argv_builder_add(b, "quiet");
    argv_builder_add(b, path);
    return 0;
}
