#ifndef APP_CORE_MESSAGE_FILE_NAME_H
#define APP_CORE_MESSAGE_FILE_NAME_H

#include <stddef.h>

#include "core/message.h"

/* The name to give a message's file when saving it: the document's own
 * file name (the start of its text), else the downloaded file's name.
 * Never contains a slash. */
void message_file_name(const Message *message, char *out, size_t size);

#endif
