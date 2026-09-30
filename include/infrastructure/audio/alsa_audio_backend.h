#ifndef APP_INFRASTRUCTURE_AUDIO_ALSA_AUDIO_BACKEND_H
#define APP_INFRASTRUCTURE_AUDIO_ALSA_AUDIO_BACKEND_H

#include "contracts/i_audio_backend.h"

IAudioBackend *alsa_audio_backend_create(void);

#endif
