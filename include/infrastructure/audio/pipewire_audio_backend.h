#ifndef APP_INFRASTRUCTURE_AUDIO_PIPEWIRE_AUDIO_BACKEND_H
#define APP_INFRASTRUCTURE_AUDIO_PIPEWIRE_AUDIO_BACKEND_H

#include "contracts/i_audio_backend.h"

IAudioBackend *pipewire_audio_backend_create(void);

#endif
