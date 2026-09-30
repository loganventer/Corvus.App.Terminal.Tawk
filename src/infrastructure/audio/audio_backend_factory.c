#include "infrastructure/audio/audio_backend_factory.h"
#include "infrastructure/audio/alsa_audio_backend.h"
#include "infrastructure/audio/coreaudio_audio_backend.h"
#include "infrastructure/audio/dshow_audio_backend.h"
#include "infrastructure/audio/pipewire_audio_backend.h"
#include "infrastructure/audio/pulse_audio_backend.h"
#include "utilities/log.h"

#include <string.h>

typedef IAudioBackend *(*BackendCtor)(void);

static const struct { const char *name; BackendCtor create; } BACKENDS[] = {
    { "coreaudio", coreaudio_audio_backend_create },
    { "dshow",     dshow_audio_backend_create },
    { "pipewire",  pipewire_audio_backend_create },
    { "pulse",     pulse_audio_backend_create },
    { "alsa",      alsa_audio_backend_create },
};

#define BACKEND_COUNT ((int)(sizeof(BACKENDS) / sizeof(BACKENDS[0])))

IAudioBackend *audio_backend_factory_create(const char *name) {
    if (name && strcmp(name, "auto") != 0) {
        for (int i = 0; i < BACKEND_COUNT; i++) {
            if (strcmp(BACKENDS[i].name, name) == 0) return BACKENDS[i].create();
        }
    }
    for (int i = 0; i < BACKEND_COUNT; i++) {
        IAudioBackend *b = BACKENDS[i].create();
        if (b && b->available(b)) {
            LOG_INFO("audio backend: %s", b->name(b));
            return b;
        }
        if (b) b->destroy(b);
    }
    LOG_WARN("no audio backend available; voice notes and sounds are disabled");
    return alsa_audio_backend_create();
}
