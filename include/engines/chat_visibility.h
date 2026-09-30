#ifndef APP_ENGINES_CHAT_VISIBILITY_H
#define APP_ENGINES_CHAT_VISIBILITY_H

/* Chats tawk does not show: WhatsApp Status updates (status@broadcast) and
 * other broadcast feeds. Their messages are dropped on arrival. */
int chat_visibility_hidden(const char *jid);

#endif
