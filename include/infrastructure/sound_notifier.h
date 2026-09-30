#ifndef APP_INFRASTRUCTURE_SOUND_NOTIFIER_H
#define APP_INFRASTRUCTURE_SOUND_NOTIFIER_H

#include "contracts/i_audio_player.h"
#include "contracts/i_notifier.h"
#include "core/settings.h"

/* Plays the configured notification sound. Borrows the player and settings. */
INotifier *sound_notifier_create(IAudioPlayer *player, const Settings *settings);

#endif
