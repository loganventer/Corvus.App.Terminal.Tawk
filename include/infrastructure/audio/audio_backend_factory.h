#ifndef APP_INFRASTRUCTURE_AUDIO_AUDIO_BACKEND_FACTORY_H
#define APP_INFRASTRUCTURE_AUDIO_AUDIO_BACKEND_FACTORY_H

#include "contracts/i_audio_backend.h"

/* Creates the named backend, or detects one for "auto": CoreAudio on macOS,
 * DirectShow on Windows (MSYS2/Cygwin), then PipeWire, PulseAudio (incl. WSLg) and ALSA. Never returns NULL for
 * "auto"; the result may report available() == 0 when nothing is installed. */
IAudioBackend *audio_backend_factory_create(const char *name);

#endif
