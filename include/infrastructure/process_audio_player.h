#ifndef APP_INFRASTRUCTURE_PROCESS_AUDIO_PLAYER_H
#define APP_INFRASTRUCTURE_PROCESS_AUDIO_PLAYER_H

#include "contracts/i_audio_backend.h"
#include "contracts/i_audio_player.h"

/* Plays files with whatever command the audio backend prescribes. The
 * backend is borrowed and must outlive the player. */
IAudioPlayer *process_audio_player_create(IAudioBackend *backend);

#endif
