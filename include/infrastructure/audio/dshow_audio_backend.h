#ifndef APP_INFRASTRUCTURE_AUDIO_DSHOW_AUDIO_BACKEND_H
#define APP_INFRASTRUCTURE_AUDIO_DSHOW_AUDIO_BACKEND_H

#include "contracts/i_audio_backend.h"

/* Windows builds (MSYS2 / Cygwin): ffmpeg DirectShow capture, ffplay playback. */
IAudioBackend *dshow_audio_backend_create(void);

#endif
