#ifndef APP_ENGINES_SOUND_NAME_H
#define APP_ENGINES_SOUND_NAME_H

#include <stddef.h>

/* Whether a file name is a sound tawk can play (.wav or .ogg, any case). */
int  sound_name_is_playable(const char *file_name);
/* The name to show for a sound file: its file name without folder or extension. */
void sound_name_label(const char *path, char *out, size_t size);

#endif
