#ifndef APP_ENGINES_MESSAGE_ID_GENERATOR_H
#define APP_ENGINES_MESSAGE_ID_GENERATOR_H

#include <stddef.h>

/* WhatsApp Web style id ("3EB0" + 18 random hex digits) from the OS CSPRNG. */
int message_id_generate(char *out, size_t size);

#endif
