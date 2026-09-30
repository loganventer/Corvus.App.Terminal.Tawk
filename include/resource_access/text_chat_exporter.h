#ifndef APP_RESOURCE_ACCESS_TEXT_CHAT_EXPORTER_H
#define APP_RESOURCE_ACCESS_TEXT_CHAT_EXPORTER_H

#include "contracts/i_chat_exporter.h"

/* Exports in WhatsApp's own text format ("[29/09/2026, 14:05] Name: text"),
 * as "tawk chat with NAME.txt", or with media as a folder holding the text
 * file and copies of the downloaded files. Never overwrites anything. */
IChatExporter *text_chat_exporter_create(void);

#endif
