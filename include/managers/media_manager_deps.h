#ifndef APP_MANAGERS_MEDIA_MANAGER_DEPS_H
#define APP_MANAGERS_MEDIA_MANAGER_DEPS_H

#include "contracts/i_audio_backend.h"
#include "contracts/i_audio_player.h"
#include "contracts/i_audio_recorder.h"
#include "contracts/i_camera.h"
#include "contracts/i_media_opener.h"
#include "core/settings.h"

typedef struct MediaManagerDeps {
    IMediaOpener   *opener;
    IAudioPlayer   *voice_player;
    IAudioRecorder *recorder;
    ICamera        *camera;
    IAudioBackend  *audio;          /* the sound for videos; may be NULL */
    const Settings *settings;
} MediaManagerDeps;

#endif
