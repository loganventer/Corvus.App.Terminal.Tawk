#ifndef APP_CORE_SOUND_OPTION_H
#define APP_CORE_SOUND_OPTION_H

/* One sound file a notification can use. */
typedef struct SoundOption {
    char id[64];       /* the file name without its extension, as shown */
    char path[512];
} SoundOption;

#endif
