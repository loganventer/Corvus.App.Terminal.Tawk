#ifndef APP_INFRASTRUCTURE_AUDIO_FFPLAY_FALLBACK_H
#define APP_INFRASTRUCTURE_AUDIO_FFPLAY_FALLBACK_H

#include "utilities/argv_builder.h"

/* Adds a headless ffplay invocation (plays Ogg/Opus everywhere). Returns 0
 * when ffplay is on PATH, -1 otherwise. */
int ffplay_fallback_add(ArgvBuilder *b, const char *path);

#endif
