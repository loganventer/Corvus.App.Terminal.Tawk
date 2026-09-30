#ifndef APP_ENGINES_STATUS_BACKGROUND_PALETTE_H
#define APP_ENGINES_STATUS_BACKGROUND_PALETTE_H

#include <stdint.h>

/* The background colours WhatsApp offers for text statuses, as ARGB. */
int      status_background_count(void);
uint32_t status_background_at(int index);   /* wraps around */
/* The colour's name for people ("Teal"), wrapping like status_background_at. */
const char *status_background_name(int index);

#endif
