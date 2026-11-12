#include "clients/tui/tab_chat_name.h"

const char *tab_chat_name(const Chat *chat, int veiled) {
    if (!chat) return "";
    return veiled ? "\xF0\x9F\x94\x92" : chat->name;
}
