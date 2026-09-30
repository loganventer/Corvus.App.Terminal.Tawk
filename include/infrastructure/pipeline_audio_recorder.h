#ifndef APP_INFRASTRUCTURE_PIPELINE_AUDIO_RECORDER_H
#define APP_INFRASTRUCTURE_PIPELINE_AUDIO_RECORDER_H

#include "contracts/i_audio_backend.h"
#include "contracts/i_audio_recorder.h"

/* Records as a pipeline: the backend's capture command writes raw PCM into
 * ffmpeg, which encodes 48 kHz mono Ogg/Opus (the WhatsApp voice note
 * format). The backend is borrowed and must outlive the recorder. */
IAudioRecorder *pipeline_audio_recorder_create(IAudioBackend *backend);

#endif
