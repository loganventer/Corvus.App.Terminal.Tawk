#ifndef APP_INFRASTRUCTURE_AUDIO_COREAUDIO_AUDIO_BACKEND_H
#define APP_INFRASTRUCTURE_AUDIO_COREAUDIO_AUDIO_BACKEND_H

#include "contracts/i_audio_backend.h"

IAudioBackend *coreaudio_audio_backend_create(void);

#endif
